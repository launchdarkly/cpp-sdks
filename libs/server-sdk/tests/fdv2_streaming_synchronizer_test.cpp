#include <gtest/gtest.h>

#include <data_systems/fdv2/streaming_synchronizer.hpp>

#include <launchdarkly/data_model/selector.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/server_side/config/builders/all_builders.hpp>
#include <launchdarkly/sse/client.hpp>
#include <launchdarkly/sse/error.hpp>
#include <launchdarkly/sse/event.hpp>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/http.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <thread>

namespace launchdarkly::server_side::data_systems {

// Test peer providing access to private State internals via the
// friend declaration in streaming_synchronizer.hpp. Tests drive the State's
// per-event / per-error / per-connect entry points directly so that no real
// SSE connection is required.
class FDv2StreamingSynchronizerTestPeer {
   public:
    static void OnEvent(FDv2StreamingSynchronizer& sync,
                        sse::Event const& event) {
        sync.state_->OnEvent(event);
    }
    static void OnError(FDv2StreamingSynchronizer& sync,
                        sse::Error const& error) {
        sync.state_->OnError(error);
    }
    static void OnConnect(
        FDv2StreamingSynchronizer& sync,
        boost::beast::http::request<boost::beast::http::string_body>* req) {
        sync.state_->OnConnect(req);
    }
    static void MarkStarted(FDv2StreamingSynchronizer& sync) {
        std::lock_guard lock(sync.state_->mutex_);
        sync.state_->started_ = true;
    }
    static void SetBaseUrl(FDv2StreamingSynchronizer& sync,
                           boost::urls::url url) {
        std::lock_guard lock(sync.state_->mutex_);
        sync.state_->base_url_ = std::move(url);
    }
    static void SetLatestSelector(FDv2StreamingSynchronizer& sync,
                                  data_model::Selector selector) {
        std::lock_guard lock(sync.state_->mutex_);
        sync.state_->latest_selector_ = std::move(selector);
    }
    static void SetSseClient(FDv2StreamingSynchronizer& sync,
                             std::shared_ptr<sse::Client> client) {
        std::lock_guard lock(sync.state_->mutex_);
        sync.state_->sse_client_ = std::move(client);
    }
};

}  // namespace launchdarkly::server_side::data_systems

using namespace launchdarkly;
using namespace launchdarkly::server_side::data_interfaces;
using namespace launchdarkly::server_side::data_systems;
using namespace std::chrono_literals;

namespace {

Logger MakeNullLogger() {
    struct NullBackend : ILogBackend {
        bool Enabled(LogLevel) noexcept override { return false; }
        void Write(LogLevel, std::string) noexcept override {}
    };
    return Logger{std::make_shared<NullBackend>()};
}

class IoContextRunner {
   public:
    IoContextRunner() : work_guard_(boost::asio::make_work_guard(ioc_)) {
        thread_ = std::thread([this] { ioc_.run(); });
    }
    ~IoContextRunner() {
        work_guard_.reset();
        ioc_.stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }
    boost::asio::io_context& context() { return ioc_; }

   private:
    boost::asio::io_context ioc_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work_guard_;
    std::thread thread_;
};

config::shared::built::ServiceEndpoints MakeEndpoints(std::string streaming) {
    return config::shared::built::ServiceEndpoints(
        "polling", std::move(streaming), "events");
}

config::shared::built::HttpProperties MakeHttpProperties() {
    return launchdarkly::server_side::config::builders::HttpPropertiesBuilder()
        .Build();
}

// Records calls to the sse::Client interface. Used to verify that the
// synchronizer drives the underlying SSE connection correctly without
// requiring a real network client.
class MockSseClient : public sse::Client {
   public:
    void async_connect() override { ++connect_count_; }
    void async_shutdown(std::function<void()> completion) override {
        ++shutdown_count_;
        if (completion) {
            completion();
        }
    }
    void async_restart(std::string const& reason) override {
        ++restart_count_;
        last_restart_reason_ = reason;
    }

    int connect_count_ = 0;
    int shutdown_count_ = 0;
    int restart_count_ = 0;
    std::string last_restart_reason_;
};

}  // namespace

// ============================================================================
// Lifecycle
// ============================================================================

TEST(FDv2StreamingSynchronizerTest, NextBadEndpointUrlReturnsTerminalError) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger, MakeEndpoints("not a url"),
        MakeHttpProperties(), std::nullopt, 1s);

    // Act: trigger setup with a malformed streaming URL. URL parsing happens
    // inside EnsureStarted on the first Next call.
    auto future = synchronizer.Next(2s, data_model::Selector{});
    auto result = future.WaitForResult(2s);

    // Assert: TerminalError tells the orchestrator not to retry, which is the
    // correct response to a misconfigured endpoint URL.
    ASSERT_TRUE(result.has_value());
    auto* terminal =
        std::get_if<FDv2SourceResult::TerminalError>(&result->value);
    ASSERT_NE(terminal, nullptr);
    EXPECT_EQ(terminal->error.Kind(),
              FDv2SourceResult::ErrorInfo::ErrorKind::kNetworkError);
}

TEST(FDv2StreamingSynchronizerTest, CloseBeforeNextReturnsShutdown) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeEndpoints("http://localhost"), MakeHttpProperties(), std::nullopt,
        1s);
    synchronizer.Close();

    // Act: call Next on an already-closed synchronizer.
    auto future = synchronizer.Next(5s, data_model::Selector{});
    auto result = future.WaitForResult(2s);

    // Assert: Shutdown is delivered immediately (the outer Next short-circuits
    // when close_promise_ is already resolved).
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Shutdown>(result->value));
}

TEST(FDv2StreamingSynchronizerTest, CloseDuringPendingNextResolvesShutdown) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeEndpoints("http://localhost"), MakeHttpProperties(), std::nullopt,
        1s);

    // Skip the SSE setup; we want Next to be pending purely on the
    // close/timeout race, not on real network activity.
    FDv2StreamingSynchronizerTestPeer::MarkStarted(synchronizer);

    // Act: start a pending Next, then Close while it is still pending.
    auto future = synchronizer.Next(5s, data_model::Selector{});
    synchronizer.Close();
    auto result = future.WaitForResult(2s);

    // Assert: Close resolves the pending Next with Shutdown by winning the
    // close-vs-timeout-vs-result race in the outer Next.
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Shutdown>(result->value));
}

TEST(FDv2StreamingSynchronizerTest, NextTimeoutReturnsTimeout) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeEndpoints("http://localhost"), MakeHttpProperties(), std::nullopt,
        1s);

    FDv2StreamingSynchronizerTestPeer::MarkStarted(synchronizer);

    // Act: call Next with a short timeout and never deliver any event.
    auto future = synchronizer.Next(100ms, data_model::Selector{});
    auto result = future.WaitForResult(2s);

    // Assert: the timeout future wins the race and Next resolves with Timeout.
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Timeout>(result->value));
}

// ============================================================================
// on_connect hook — URL/target construction
// ============================================================================

TEST(FDv2StreamingSynchronizerTest, OnConnectEmptySelectorNoBasisParam) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeEndpoints("https://stream.example.com"), MakeHttpProperties(),
        std::nullopt, 1s);

    boost::urls::url base =
        boost::urls::parse_uri("https://stream.example.com").value();
    base.segments().push_back("sdk");
    base.segments().push_back("stream");
    FDv2StreamingSynchronizerTestPeer::SetBaseUrl(synchronizer, base);

    boost::beast::http::request<boost::beast::http::string_body> req;

    // Act: invoke the on_connect hook with no selector configured.
    FDv2StreamingSynchronizerTestPeer::OnConnect(synchronizer, &req);

    // Assert: no basis query param appears in the target. The path itself is
    // /sdk/stream as built from the streaming base URL.
    EXPECT_EQ(req.target(), "/sdk/stream");
}

TEST(FDv2StreamingSynchronizerTest, OnConnectWithSelectorAppendsBasis) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeEndpoints("https://stream.example.com"), MakeHttpProperties(),
        std::nullopt, 1s);

    boost::urls::url base =
        boost::urls::parse_uri("https://stream.example.com").value();
    base.segments().push_back("sdk");
    base.segments().push_back("stream");
    FDv2StreamingSynchronizerTestPeer::SetBaseUrl(synchronizer, base);
    FDv2StreamingSynchronizerTestPeer::SetLatestSelector(
        synchronizer,
        data_model::Selector{data_model::Selector::State{1, "abc"}});

    boost::beast::http::request<boost::beast::http::string_body> req;

    // Act: invoke the on_connect hook with a non-empty selector.
    FDv2StreamingSynchronizerTestPeer::OnConnect(synchronizer, &req);

    // Assert: the selector's state string is encoded as the basis query param.
    EXPECT_EQ(req.target(), "/sdk/stream?basis=abc");
}

TEST(FDv2StreamingSynchronizerTest, OnConnectWithFilterKeyAppendsFilter) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeEndpoints("https://stream.example.com"), MakeHttpProperties(),
        std::string("my-filter"), 1s);

    boost::urls::url base =
        boost::urls::parse_uri("https://stream.example.com").value();
    base.segments().push_back("sdk");
    base.segments().push_back("stream");
    FDv2StreamingSynchronizerTestPeer::SetBaseUrl(synchronizer, base);

    boost::beast::http::request<boost::beast::http::string_body> req;

    // Act: invoke the on_connect hook with a configured filter key but no
    // selector.
    FDv2StreamingSynchronizerTestPeer::OnConnect(synchronizer, &req);

    // Assert: the filter key is encoded as the filter query param.
    EXPECT_EQ(req.target(), "/sdk/stream?filter=my-filter");
}

TEST(FDv2StreamingSynchronizerTest, OnConnectReconnectUsesLatestSelector) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeEndpoints("https://stream.example.com"), MakeHttpProperties(),
        std::nullopt, 1s);

    boost::urls::url base =
        boost::urls::parse_uri("https://stream.example.com").value();
    base.segments().push_back("sdk");
    base.segments().push_back("stream");
    FDv2StreamingSynchronizerTestPeer::SetBaseUrl(synchronizer, base);

    FDv2StreamingSynchronizerTestPeer::SetLatestSelector(
        synchronizer,
        data_model::Selector{data_model::Selector::State{1, "s0"}});
    boost::beast::http::request<boost::beast::http::string_body> req1;
    FDv2StreamingSynchronizerTestPeer::OnConnect(synchronizer, &req1);

    FDv2StreamingSynchronizerTestPeer::SetLatestSelector(
        synchronizer,
        data_model::Selector{data_model::Selector::State{2, "s1"}});

    boost::beast::http::request<boost::beast::http::string_body> req2;

    // Act: invoke the on_connect hook a second time after the stored selector
    // has been updated (modeling a reconnect after a payload-transferred event
    // has arrived).
    FDv2StreamingSynchronizerTestPeer::OnConnect(synchronizer, &req2);

    // Assert: the second connection's basis reflects the updated selector,
    // not the value captured at construction or first connect.
    EXPECT_EQ(req1.target(), "/sdk/stream?basis=s0");
    EXPECT_EQ(req2.target(), "/sdk/stream?basis=s1");
}

TEST(FDv2StreamingSynchronizerTest, OnConnectSelectorStateIsPercentEncoded) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeEndpoints("https://stream.example.com"), MakeHttpProperties(),
        std::nullopt, 1s);

    boost::urls::url base =
        boost::urls::parse_uri("https://stream.example.com").value();
    base.segments().push_back("sdk");
    base.segments().push_back("stream");
    FDv2StreamingSynchronizerTestPeer::SetBaseUrl(synchronizer, base);
    FDv2StreamingSynchronizerTestPeer::SetLatestSelector(
        synchronizer,
        data_model::Selector{data_model::Selector::State{1, "a&b"}});

    boost::beast::http::request<boost::beast::http::string_body> req;

    // Act: invoke the on_connect hook with a selector state containing '&',
    // which would terminate the basis value if left raw.
    FDv2StreamingSynchronizerTestPeer::OnConnect(synchronizer, &req);

    // Assert: '&' is percent-encoded into the basis query param.
    EXPECT_EQ(req.target(), "/sdk/stream?basis=a%26b");
}

// ============================================================================
// SSE event translation
// ============================================================================

TEST(FDv2StreamingSynchronizerTest, FullChangesetEventsReturnsChangeSet) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeEndpoints("http://localhost"), MakeHttpProperties(), std::nullopt,
        1s);
    FDv2StreamingSynchronizerTestPeer::MarkStarted(synchronizer);

    sse::Event server_intent("server-intent",
                             R"({"payloads":[{"id":"p1","target":1,)"
                             R"("intentCode":"xfer-full"}]})");
    sse::Event put_object(
        "put-object",
        R"({"version":1,"kind":"flag","key":"my-flag","object":)"
        R"({"key":"my-flag","on":true,"fallthrough":{"variation":0},)"
        R"("variations":[true,false],"version":1}})");
    sse::Event payload_transferred("payload-transferred",
                                   R"({"state":"abc","version":1})");

    // Act: drive a full changeset (intent + put + transferred) through OnEvent
    // and then read the queued result via the public Next API.
    FDv2StreamingSynchronizerTestPeer::OnEvent(synchronizer, server_intent);
    FDv2StreamingSynchronizerTestPeer::OnEvent(synchronizer, put_object);
    FDv2StreamingSynchronizerTestPeer::OnEvent(synchronizer,
                                               payload_transferred);
    auto future = synchronizer.Next(2s, data_model::Selector{});
    auto result = future.WaitForResult(2s);

    // Assert: a ChangeSet result is delivered, and the translated payload
    // contains the put item from the put-object event.
    ASSERT_TRUE(result.has_value());
    auto* change_set = std::get_if<FDv2SourceResult::ChangeSet>(&result->value);
    ASSERT_NE(change_set, nullptr);
    EXPECT_FALSE(change_set->change_set.data.empty());
}

TEST(FDv2StreamingSynchronizerTest, GoodbyeEventReturnsGoodbye) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeEndpoints("http://localhost"), MakeHttpProperties(), std::nullopt,
        1s);
    FDv2StreamingSynchronizerTestPeer::MarkStarted(synchronizer);

    sse::Event goodbye("goodbye", R"({"reason":"bye"})");

    // Act: deliver a goodbye event through OnEvent and read the queued result.
    FDv2StreamingSynchronizerTestPeer::OnEvent(synchronizer, goodbye);
    auto future = synchronizer.Next(2s, data_model::Selector{});
    auto result = future.WaitForResult(2s);

    // Assert: a Goodbye result with the wire reason is delivered, signaling
    // the orchestrator to rotate sources.
    ASSERT_TRUE(result.has_value());
    auto* g = std::get_if<FDv2SourceResult::Goodbye>(&result->value);
    ASSERT_NE(g, nullptr);
    ASSERT_TRUE(g->reason.has_value());
    EXPECT_EQ(*g->reason, "bye");
}

TEST(FDv2StreamingSynchronizerTest, GoodbyeEventTriggersAsyncRestart) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeEndpoints("http://localhost"), MakeHttpProperties(), std::nullopt,
        1s);
    FDv2StreamingSynchronizerTestPeer::MarkStarted(synchronizer);

    auto mock_client = std::make_shared<MockSseClient>();
    FDv2StreamingSynchronizerTestPeer::SetSseClient(synchronizer, mock_client);

    sse::Event goodbye("goodbye", R"({"reason":"bye"})");

    // Act: deliver a goodbye event and drain the resulting Goodbye result.
    FDv2StreamingSynchronizerTestPeer::OnEvent(synchronizer, goodbye);
    auto future = synchronizer.Next(2s, data_model::Selector{});
    auto result = future.WaitForResult(2s);
    ASSERT_TRUE(result.has_value());

    // Assert: the Goodbye handler drove the SSE client to restart with the
    // documented reason string. Without this, the server's "we're about to
    // disconnect" signal would lead to a stalled connection rather than a
    // controlled reconnect.
    EXPECT_EQ(mock_client->restart_count_, 1);
    EXPECT_EQ(mock_client->last_restart_reason_, "FDv2 goodbye received");
}

TEST(FDv2StreamingSynchronizerTest,
     GoodbyeMidPayloadDiscardsAccumulatedAndAcceptsFreshChangeset) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeEndpoints("http://localhost"), MakeHttpProperties(), std::nullopt,
        1s);
    FDv2StreamingSynchronizerTestPeer::MarkStarted(synchronizer);

    // Begin accumulating a payload that we'll abandon mid-flight via Goodbye.
    sse::Event server_intent_one("server-intent",
                                 R"({"payloads":[{"id":"p1","target":1,)"
                                 R"("intentCode":"xfer-full"}]})");
    sse::Event abandoned_put(
        "put-object",
        R"({"version":1,"kind":"flag","key":"abandoned","object":)"
        R"({"key":"abandoned","on":true,"fallthrough":{"variation":0},)"
        R"("variations":[true,false],"version":1}})");
    FDv2StreamingSynchronizerTestPeer::OnEvent(synchronizer,
                                               server_intent_one);
    FDv2StreamingSynchronizerTestPeer::OnEvent(synchronizer, abandoned_put);

    // Goodbye arrives mid-payload; expect a Goodbye result and the partial
    // payload to be discarded.
    sse::Event goodbye("goodbye", R"({"reason":"bye"})");
    FDv2StreamingSynchronizerTestPeer::OnEvent(synchronizer, goodbye);
    auto goodbye_result =
        synchronizer.Next(2s, data_model::Selector{}).WaitForResult(2s);
    ASSERT_TRUE(goodbye_result.has_value());
    ASSERT_NE(std::get_if<FDv2SourceResult::Goodbye>(&goodbye_result->value),
              nullptr);

    // Drive a fresh full changeset through. The protocol handler must be back
    // in a clean state — neither carrying the abandoned put nor stuck in the
    // previous accumulating state.
    sse::Event server_intent_two("server-intent",
                                 R"({"payloads":[{"id":"p2","target":2,)"
                                 R"("intentCode":"xfer-full"}]})");
    sse::Event fresh_put(
        "put-object",
        R"({"version":2,"kind":"flag","key":"fresh","object":)"
        R"({"key":"fresh","on":true,"fallthrough":{"variation":0},)"
        R"("variations":[true,false],"version":2}})");
    sse::Event payload_transferred("payload-transferred",
                                   R"({"state":"abc","version":2})");
    FDv2StreamingSynchronizerTestPeer::OnEvent(synchronizer,
                                               server_intent_two);
    FDv2StreamingSynchronizerTestPeer::OnEvent(synchronizer, fresh_put);
    FDv2StreamingSynchronizerTestPeer::OnEvent(synchronizer,
                                               payload_transferred);
    auto changeset_result =
        synchronizer.Next(2s, data_model::Selector{}).WaitForResult(2s);
    ASSERT_TRUE(changeset_result.has_value());
    auto* cs =
        std::get_if<FDv2SourceResult::ChangeSet>(&changeset_result->value);
    ASSERT_NE(cs, nullptr);

    // Only the fresh put should be present; the abandoned put was discarded.
    ASSERT_EQ(cs->change_set.data.size(), 1u);
    EXPECT_EQ(cs->change_set.data[0].key, "fresh");
}

TEST(FDv2StreamingSynchronizerTest, ServerErrorEventReturnsInterrupted) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeEndpoints("http://localhost"), MakeHttpProperties(), std::nullopt,
        1s);
    FDv2StreamingSynchronizerTestPeer::MarkStarted(synchronizer);

    sse::Event server_error("error", R"({"id":"abc","reason":"oops"})");

    // Act: deliver an FDv2 server-side error event.
    FDv2StreamingSynchronizerTestPeer::OnEvent(synchronizer, server_error);
    auto future = synchronizer.Next(2s, data_model::Selector{});
    auto result = future.WaitForResult(2s);

    // Assert: the error is reported as Interrupted{kErrorResponse}, with the
    // formatted retry-will-occur message containing both the payload id and
    // the server reason.
    ASSERT_TRUE(result.has_value());
    auto* interrupted =
        std::get_if<FDv2SourceResult::Interrupted>(&result->value);
    ASSERT_NE(interrupted, nullptr);
    EXPECT_EQ(interrupted->error.Kind(),
              FDv2SourceResult::ErrorInfo::ErrorKind::kErrorResponse);
    EXPECT_NE(interrupted->error.Message().find("abc"), std::string::npos);
    EXPECT_NE(interrupted->error.Message().find("oops"), std::string::npos);
    EXPECT_NE(interrupted->error.Message().find("Automatic retry"),
              std::string::npos);
}

TEST(FDv2StreamingSynchronizerTest, MalformedJsonEventReturnsInterrupted) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeEndpoints("http://localhost"), MakeHttpProperties(), std::nullopt,
        1s);
    FDv2StreamingSynchronizerTestPeer::MarkStarted(synchronizer);

    sse::Event bad_event("server-intent", "this is not json");

    // Act: deliver an event whose data field cannot be parsed as JSON.
    FDv2StreamingSynchronizerTestPeer::OnEvent(synchronizer, bad_event);
    auto future = synchronizer.Next(2s, data_model::Selector{});
    auto result = future.WaitForResult(2s);

    // Assert: the synchronizer reports Interrupted{kInvalidData} so the
    // orchestrator knows the stream produced unparseable bytes.
    ASSERT_TRUE(result.has_value());
    auto* interrupted =
        std::get_if<FDv2SourceResult::Interrupted>(&result->value);
    ASSERT_NE(interrupted, nullptr);
    EXPECT_EQ(interrupted->error.Kind(),
              FDv2SourceResult::ErrorInfo::ErrorKind::kInvalidData);
}

TEST(FDv2StreamingSynchronizerTest, HeartbeatEventNoResultDelivered) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeEndpoints("http://localhost"), MakeHttpProperties(), std::nullopt,
        1s);
    FDv2StreamingSynchronizerTestPeer::MarkStarted(synchronizer);

    sse::Event heartbeat("heartbeat", R"({})");

    // Act: deliver a heartbeat event, then call Next with a short timeout.
    FDv2StreamingSynchronizerTestPeer::OnEvent(synchronizer, heartbeat);
    auto future = synchronizer.Next(100ms, data_model::Selector{});
    auto result = future.WaitForResult(2s);

    // Assert: the heartbeat does not produce any FDv2SourceResult, so Next
    // resolves with Timeout instead.
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Timeout>(result->value));
}

TEST(FDv2StreamingSynchronizerTest, TranslationFailureReturnsInterrupted) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeEndpoints("http://localhost"), MakeHttpProperties(), std::nullopt,
        1s);
    FDv2StreamingSynchronizerTestPeer::MarkStarted(synchronizer);

    // A non-empty segment object missing required fields triggers a schema
    // failure in the translator; matches the segment shape used by
    // fdv2_changeset_translation_test's MalformedSegmentAbortsTranslation.
    sse::Event server_intent("server-intent",
                             R"({"payloads":[{"id":"p1","target":1,)"
                             R"("intentCode":"xfer-full"}]})");
    sse::Event put_object(
        "put-object",
        R"({"version":1,"kind":"segment","key":"bad-seg","object":)"
        R"({"key":"bad-seg"}})");
    sse::Event payload_transferred("payload-transferred",
                                   R"({"state":"abc","version":1})");

    // Act: drive a full changeset whose put-object payload won't deserialize
    // into a Segment.
    FDv2StreamingSynchronizerTestPeer::OnEvent(synchronizer, server_intent);
    FDv2StreamingSynchronizerTestPeer::OnEvent(synchronizer, put_object);
    FDv2StreamingSynchronizerTestPeer::OnEvent(synchronizer,
                                               payload_transferred);
    auto future = synchronizer.Next(2s, data_model::Selector{});
    auto result = future.WaitForResult(2s);

    // Assert: the changeset is rejected with Interrupted{kInvalidData}.
    ASSERT_TRUE(result.has_value());
    auto* interrupted =
        std::get_if<FDv2SourceResult::Interrupted>(&result->value);
    ASSERT_NE(interrupted, nullptr);
    EXPECT_EQ(interrupted->error.Kind(),
              FDv2SourceResult::ErrorInfo::ErrorKind::kInvalidData);
}

// ============================================================================
// SSE error handling
// ============================================================================

TEST(FDv2StreamingSynchronizerTest,
     UnrecoverableStatus500ReturnsTerminalError) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeEndpoints("http://localhost"), MakeHttpProperties(), std::nullopt,
        1s);
    FDv2StreamingSynchronizerTestPeer::MarkStarted(synchronizer);

    sse::Error error{sse::errors::UnrecoverableClientError{
        boost::beast::http::status::internal_server_error}};

    // Act: deliver an unrecoverable HTTP 500 error from the SSE client.
    FDv2StreamingSynchronizerTestPeer::OnError(synchronizer, error);
    auto future = synchronizer.Next(2s, data_model::Selector{});
    auto result = future.WaitForResult(2s);

    // Assert: the synchronizer reports TerminalError{kErrorResponse} carrying
    // the HTTP status, telling the orchestrator to stop retrying this source.
    ASSERT_TRUE(result.has_value());
    auto* terminal =
        std::get_if<FDv2SourceResult::TerminalError>(&result->value);
    ASSERT_NE(terminal, nullptr);
    EXPECT_EQ(terminal->error.Kind(),
              FDv2SourceResult::ErrorInfo::ErrorKind::kErrorResponse);
    EXPECT_EQ(terminal->error.StatusCode(), 500u);
}

TEST(FDv2StreamingSynchronizerTest, RecoverableReadTimeoutReturnsInterrupted) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeEndpoints("http://localhost"), MakeHttpProperties(), std::nullopt,
        1s);
    FDv2StreamingSynchronizerTestPeer::MarkStarted(synchronizer);

    sse::Error error{sse::errors::ReadTimeout{std::chrono::milliseconds(100)}};

    // Act: deliver a recoverable read-timeout error from the SSE client.
    FDv2StreamingSynchronizerTestPeer::OnError(synchronizer, error);
    auto future = synchronizer.Next(2s, data_model::Selector{});
    auto result = future.WaitForResult(2s);

    // Assert: the synchronizer reports Interrupted{kNetworkError} so the
    // orchestrator treats this as a recoverable blip rather than a terminal
    // failure.
    ASSERT_TRUE(result.has_value());
    auto* interrupted =
        std::get_if<FDv2SourceResult::Interrupted>(&result->value);
    ASSERT_NE(interrupted, nullptr);
    EXPECT_EQ(interrupted->error.Kind(),
              FDv2SourceResult::ErrorInfo::ErrorKind::kNetworkError);
}
