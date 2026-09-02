#include <gtest/gtest.h>

#include <data_sources/fdv2/streaming_synchronizer.hpp>

#include <launchdarkly/config/shared/defaults.hpp>
#include <launchdarkly/data_model/selector.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/sse/error.hpp>
#include <launchdarkly/sse/event.hpp>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/http.hpp>
#include <boost/url/parse.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <thread>

namespace launchdarkly::client_side::data_sources {

// Drives the State's per-event, per-error, and per-connect entry points
// directly, so that no real SSE connection is required.
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
    static void OnResponse(
        FDv2StreamingSynchronizer& sync,
        boost::beast::http::response_header<> const& headers) {
        sync.state_->OnResponse(headers);
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

}  // namespace launchdarkly::client_side::data_sources

using namespace launchdarkly;
using namespace launchdarkly::client_side::data_sources;
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

FDv2RequestConfig MakeConfig(
    std::string base_url,
    FDv2ContextTransport transport = FDv2ContextTransport::kGetPath,
    bool with_reasons = false) {
    return FDv2RequestConfig{
        std::move(base_url),
        config::shared::Defaults<config::shared::ClientSDK>::HttpProperties(),
        R"({"kind":"user","key":"user-key"})", transport, with_reasons};
}

// Records calls to the sse::Client interface, so tests can verify how the
// synchronizer drives the connection without a real network client.
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

boost::beast::http::response_header<> MakeResponseHeaders(
    std::vector<std::pair<std::string, std::string>> const& headers) {
    boost::beast::http::response_header<> result;
    for (auto const& [name, value] : headers) {
        result.set(name, value);
    }
    return result;
}

}  // namespace

// ============================================================================
// Lifecycle
// ============================================================================

TEST(ClientFDv2StreamingSynchronizerTest, UnparseableEndpointIsTerminal) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(runner.context().get_executor(),
                                           logger, MakeConfig("not a url"),
                                           MakeConfig("http://localhost"), 1s);

    auto result = synchronizer.Next(data_model::Selector{}).WaitForResult(2s);

    ASSERT_TRUE(result.has_value());
    auto* terminal =
        std::get_if<FDv2SourceResult::TerminalError>(&result->value);
    ASSERT_NE(nullptr, terminal);
    EXPECT_EQ(FDv2SourceResult::ErrorInfo::ErrorKind::kNetworkError,
              terminal->error.Kind());
}

TEST(ClientFDv2StreamingSynchronizerTest, NextAfterCloseIsShutdown) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger, MakeConfig("http://localhost"),
        MakeConfig("http://localhost"), 1s);
    synchronizer.Close();

    auto result = synchronizer.Next(data_model::Selector{}).WaitForResult(2s);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Shutdown>(result->value));
}

TEST(ClientFDv2StreamingSynchronizerTest, CloseUnblocksAPendingNext) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger, MakeConfig("http://localhost"),
        MakeConfig("http://localhost"), 1s);

    // Skip the SSE setup, so that Next is pending purely on the close race
    // rather than on real network activity.
    FDv2StreamingSynchronizerTestPeer::MarkStarted(synchronizer);

    auto future = synchronizer.Next(data_model::Selector{});
    synchronizer.Close();
    auto result = future.WaitForResult(2s);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Shutdown>(result->value));
}

// ============================================================================
// Request construction
// ============================================================================

TEST(ClientFDv2StreamingSynchronizerTest, GetTargetCarriesTheEncodedContext) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;
    auto client = std::make_shared<MockSseClient>();

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeConfig("https://stream.example.com"),
        MakeConfig("http://localhost"), 1s);

    // The connection is not made, but the target is built during setup.
    synchronizer.Next(data_model::Selector{});
    boost::beast::http::request<boost::beast::http::string_body> req;
    FDv2StreamingSynchronizerTestPeer::OnConnect(synchronizer, &req);

    EXPECT_EQ("/sdk/stream/eval/eyJraW5kIjoidXNlciIsImtleSI6InVzZXIta2V5In0=",
              req.target());
}

TEST(ClientFDv2StreamingSynchronizerTest, PostTargetOmitsTheContext) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeConfig("https://stream.example.com",
                   FDv2ContextTransport::kPostBody),
        MakeConfig("http://localhost"), 1s);

    synchronizer.Next(data_model::Selector{});
    boost::beast::http::request<boost::beast::http::string_body> req;
    FDv2StreamingSynchronizerTestPeer::OnConnect(synchronizer, &req);

    EXPECT_EQ("/sdk/stream/eval", req.target());
}

TEST(ClientFDv2StreamingSynchronizerTest, TargetCarriesWithReasons) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeConfig("https://stream.example.com",
                   FDv2ContextTransport::kPostBody,
                   /* with_reasons= */ true),
        MakeConfig("http://localhost"), 1s);

    synchronizer.Next(data_model::Selector{});
    boost::beast::http::request<boost::beast::http::string_body> req;
    FDv2StreamingSynchronizerTestPeer::OnConnect(synchronizer, &req);

    EXPECT_EQ("/sdk/stream/eval?withReasons=true", req.target());
}

TEST(ClientFDv2StreamingSynchronizerTest, EmptySelectorSendsNoBasis) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeConfig("https://stream.example.com",
                   FDv2ContextTransport::kPostBody),
        MakeConfig("http://localhost"), 1s);

    boost::urls::url base =
        boost::urls::parse_uri("https://stream.example.com/sdk/stream/eval")
            .value();
    FDv2StreamingSynchronizerTestPeer::SetBaseUrl(synchronizer, base);

    boost::beast::http::request<boost::beast::http::string_body> req;
    FDv2StreamingSynchronizerTestPeer::OnConnect(synchronizer, &req);

    EXPECT_EQ("/sdk/stream/eval", req.target());
}

// Each connection attempt uses the freshest selector, which is why the basis
// is appended per connect rather than baked into the base URL.
TEST(ClientFDv2StreamingSynchronizerTest, SelectorIsSentAsTheBasisPerConnect) {
    auto logger = MakeNullLogger();
    IoContextRunner runner;

    FDv2StreamingSynchronizer synchronizer(
        runner.context().get_executor(), logger,
        MakeConfig("https://stream.example.com",
                   FDv2ContextTransport::kPostBody),
        MakeConfig("http://localhost"), 1s);

    boost::urls::url base =
        boost::urls::parse_uri("https://stream.example.com/sdk/stream/eval")
            .value();
    FDv2StreamingSynchronizerTestPeer::SetBaseUrl(synchronizer, base);
    FDv2StreamingSynchronizerTestPeer::SetLatestSelector(
        synchronizer,
        data_model::Selector{data_model::Selector::State{3, "state-3"}});

    boost::beast::http::request<boost::beast::http::string_body> req;
    FDv2StreamingSynchronizerTestPeer::OnConnect(synchronizer, &req);

    EXPECT_EQ("/sdk/stream/eval?basis=state-3", req.target());
}

// ============================================================================
// Events
// ============================================================================

namespace {

// Builds a synchronizer that believes it is already streaming, so that tests
// can push events at it without a connection.
struct StreamingFixture {
    Logger logger = MakeNullLogger();
    IoContextRunner runner;
    std::shared_ptr<MockSseClient> client = std::make_shared<MockSseClient>();
    std::unique_ptr<FDv2StreamingSynchronizer> synchronizer;

    explicit StreamingFixture(std::string poll_base_url = "http://localhost") {
        synchronizer = std::make_unique<FDv2StreamingSynchronizer>(
            runner.context().get_executor(), logger,
            MakeConfig("https://stream.example.com"),
            MakeConfig(std::move(poll_base_url)), 1s);
        FDv2StreamingSynchronizerTestPeer::MarkStarted(*synchronizer);
        FDv2StreamingSynchronizerTestPeer::SetSseClient(*synchronizer, client);
    }

    void Push(std::string type, std::string data) {
        FDv2StreamingSynchronizerTestPeer::OnEvent(
            *synchronizer, sse::Event(std::move(type), std::move(data)));
    }

    std::optional<FDv2SourceResult> NextResult() {
        return synchronizer->Next(data_model::Selector{}).WaitForResult(2s);
    }
};

}  // namespace

TEST(ClientFDv2StreamingSynchronizerTest, FullTransferBecomesAChangeSet) {
    StreamingFixture f;

    f.Push("server-intent", R"({"payloads":[{"id":"p1","target":1,)"
                            R"("intentCode":"xfer-full"}]})");
    f.Push("put-object", R"({"version":7,"kind":"flag-eval","key":"my-flag",)"
                         R"("object":{"value":"a","variation":1}})");
    f.Push("payload-transferred", R"({"state":"abc","version":7})");

    auto result = f.NextResult();

    ASSERT_TRUE(result.has_value());
    auto* change_set = std::get_if<FDv2SourceResult::ChangeSet>(&result->value);
    ASSERT_NE(nullptr, change_set);
    ASSERT_EQ(1u, change_set->change_set.data.size());
    EXPECT_EQ("my-flag", change_set->change_set.data[0].key);
    ASSERT_TRUE(change_set->change_set.selector.value.has_value());
    EXPECT_EQ("abc", change_set->change_set.selector.value->state);
}

TEST(ClientFDv2StreamingSynchronizerTest, GoodbyeReportsAndReconnects) {
    StreamingFixture f;

    f.Push("goodbye", R"({"reason":"bye"})");
    auto result = f.NextResult();

    ASSERT_TRUE(result.has_value());
    auto* goodbye = std::get_if<FDv2SourceResult::Goodbye>(&result->value);
    ASSERT_NE(nullptr, goodbye);
    EXPECT_EQ("bye", goodbye->reason.value_or(""));
    EXPECT_EQ(1, f.client->restart_count_);
}

TEST(ClientFDv2StreamingSynchronizerTest, GoodbyeCarriesItsFallbackTtl) {
    StreamingFixture f;

    f.Push("goodbye", R"({"reason":"bye","protocolFallbackTTL":90})");
    auto result = f.NextResult();

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->fdv1_fallback.has_value());
    EXPECT_EQ(90s, result->fdv1_fallback->ttl);
}

TEST(ClientFDv2StreamingSynchronizerTest, UnparseableEventDataReconnects) {
    StreamingFixture f;

    f.Push("put-object", "{not json");
    auto result = f.NextResult();

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Interrupted>(result->value));
    EXPECT_EQ(1, f.client->restart_count_);
}

TEST(ClientFDv2StreamingSynchronizerTest,
     UntranslatableChangeSetIsInterrupted) {
    StreamingFixture f;

    f.Push("server-intent", R"({"payloads":[{"id":"p1","target":1,)"
                            R"("intentCode":"xfer-full"}]})");
    f.Push("put-object", R"({"version":7,"kind":"flag-eval","key":"my-flag",)"
                         R"("object":["not-an-object"]})");
    f.Push("payload-transferred", R"({"state":"abc","version":7})");

    auto result = f.NextResult();

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Interrupted>(result->value));
}

TEST(ClientFDv2StreamingSynchronizerTest, UnrecognizedEventIsIgnored) {
    StreamingFixture f;

    f.Push("something-new", R"({"anything":true})");

    // Nothing but a real result or Close can resolve the future.
    auto future = f.synchronizer->Next(data_model::Selector{});
    EXPECT_FALSE(future.IsFinished());
}

// A ping carries no data, so the SDK asks for the current payload. Pointing
// the poll at an unusable URL makes the answering request observable without
// a network.
TEST(ClientFDv2StreamingSynchronizerTest, PingTriggersAPoll) {
    StreamingFixture f("not a url");

    f.Push("ping", "");
    auto result = f.NextResult();

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Interrupted>(result->value));
    EXPECT_EQ(0, f.client->restart_count_);
}

// ============================================================================
// Response headers
// ============================================================================

TEST(ClientFDv2StreamingSynchronizerTest, ResultsCarryTheEnvironmentId) {
    StreamingFixture f;

    FDv2StreamingSynchronizerTestPeer::OnResponse(
        *f.synchronizer, MakeResponseHeaders({{"X-LD-EnvId", "env-1234"}}));
    f.Push("goodbye", R"({"reason":"bye"})");

    auto result = f.NextResult();

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->environment_id.has_value());
    EXPECT_EQ("env-1234", *result->environment_id);
}

TEST(ClientFDv2StreamingSynchronizerTest, ResultsCarryTheFallbackDirective) {
    StreamingFixture f;

    FDv2StreamingSynchronizerTestPeer::OnResponse(
        *f.synchronizer,
        MakeResponseHeaders(
            {{"X-LD-FD-Fallback", "true"}, {"X-LD-FD-Fallback-TTL", "120"}}));
    f.Push("server-intent", R"({"payloads":[{"id":"p1","target":1,)"
                            R"("intentCode":"xfer-full"}]})");
    f.Push("payload-transferred", R"({"state":"abc","version":7})");

    auto result = f.NextResult();

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->fdv1_fallback.has_value());
    EXPECT_EQ(120s, result->fdv1_fallback->ttl);
}

TEST(ClientFDv2StreamingSynchronizerTest, ReconnectWithoutTheHeaderClearsIt) {
    StreamingFixture f;

    FDv2StreamingSynchronizerTestPeer::OnResponse(
        *f.synchronizer, MakeResponseHeaders({{"X-LD-FD-Fallback", "true"}}));
    FDv2StreamingSynchronizerTestPeer::OnResponse(*f.synchronizer,
                                                  MakeResponseHeaders({}));
    f.Push("goodbye", R"({"reason":"bye"})");

    auto result = f.NextResult();

    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->fdv1_fallback.has_value());
}

// ============================================================================
// Errors
// ============================================================================

TEST(ClientFDv2StreamingSynchronizerTest, RecoverableSseErrorIsInterrupted) {
    StreamingFixture f;

    FDv2StreamingSynchronizerTestPeer::OnError(*f.synchronizer,
                                               sse::errors::ReadTimeout{});
    auto result = f.NextResult();

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Interrupted>(result->value));
}

TEST(ClientFDv2StreamingSynchronizerTest, UnrecoverableSseErrorIsTerminal) {
    StreamingFixture f;

    FDv2StreamingSynchronizerTestPeer::OnError(
        *f.synchronizer, sse::errors::UnrecoverableClientError{
                             boost::beast::http::status::unauthorized});
    auto result = f.NextResult();

    ASSERT_TRUE(result.has_value());
    auto* terminal =
        std::get_if<FDv2SourceResult::TerminalError>(&result->value);
    ASSERT_NE(nullptr, terminal);
    EXPECT_EQ(401u, terminal->error.StatusCode());
}
