#include <gtest/gtest.h>

#include <launchdarkly/async/promise.hpp>
#include <launchdarkly/client_side/client.hpp>
#include <launchdarkly/context_builder.hpp>

#include <chrono>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "../../server-sent-events/tests/mock_sse_server.hpp"

using namespace launchdarkly;
using namespace launchdarkly::client_side;
using namespace std::chrono_literals;

namespace {

class TestPersistence : public IPersistence {
   public:
    using StoreType =
        std::map<std::string,
                 std::map<std::string, std::optional<std::string>>>;

    explicit TestPersistence(StoreType store) : store_(std::move(store)) {}

    void Set(std::string storageNamespace,
             std::string key,
             std::string data) noexcept override {
        std::lock_guard lock{mutex_};
        store_[storageNamespace][key] = data;
    }

    void Remove(std::string storageNamespace,
                std::string key) noexcept override {
        std::lock_guard lock{mutex_};
        store_[storageNamespace].erase(key);
    }

    std::optional<std::string> Read(std::string storageNamespace,
                                    std::string key) noexcept override {
        std::lock_guard lock{mutex_};
        auto const ns = store_.find(storageNamespace);
        if (ns == store_.end()) {
            return std::nullopt;
        }
        auto const entry = ns->second.find(key);
        if (entry == ns->second.end()) {
            return std::nullopt;
        }
        return entry->second;
    }

   private:
    // The SDK reads from its own thread while the test writes from the main
    // one.
    std::mutex mutex_;
    StoreType store_;
};

// Offline mode makes no requests, so the whole FDv2 path can be exercised
// without a service to talk to.
Config OfflineFDv2Config(std::shared_ptr<IPersistence> persistence) {
    auto builder = ConfigBuilder("the-key");
    builder.DataSource().Method(
        DataSourceBuilder::FDv2().InitialMode(ConnectionMode::kOffline));
    builder.Events().Disable();
    if (persistence) {
        builder.Persistence().Custom(std::move(persistence));
    } else {
        builder.Persistence().None();
    }
    return builder.Build().value();
}

// The namespace and context id the client derives for SDK key "the-key" and
// context user:user-key.
char const* const kEnvironment =
    "LaunchDarkly_rUTcjlHPv6Vegd27YmtGYkEGkEUGaEbn5M0JYTFQUpA=";
char const* const kContextId = "CEXjZY7cHJG_ydFy7q4-YEFwVrG3_pkJwA4FAjrbfx0=";

}  // namespace

// The cache is the only source offline mode has, so a miss still starts the
// SDK with no flags, evaluating to defaults.
TEST(FDv2ClientTest, OfflineModeStartsWithNoCachedData) {
    Client client(OfflineFDv2Config(nullptr),
                  ContextBuilder().Kind("user", "user-key").Build());

    auto started = client.StartAsync();

    ASSERT_EQ(std::future_status::ready, started.wait_for(5s));
    EXPECT_TRUE(started.get());
    EXPECT_TRUE(client.Initialized());
    EXPECT_TRUE(client.AllFlags().empty());
}

TEST(FDv2ClientTest, OfflineModeEvaluatesAgainstTheCache) {
    auto persistence =
        std::make_shared<TestPersistence>(TestPersistence::StoreType{
            {kEnvironment,
             {{kContextId, R"({"treat":{"version":1,"value":"fish"}})"}}}});

    Client client(OfflineFDv2Config(persistence),
                  ContextBuilder().Kind("user", "user-key").Build());

    auto started = client.StartAsync();

    ASSERT_EQ(std::future_status::ready, started.wait_for(5s));
    EXPECT_TRUE(started.get());
    EXPECT_EQ("fish", client.StringVariation("treat", "chicken"));
}

// During identify, the old stream can deliver one more update. The first
// request for the new context must not contain the selector from that update.
TEST(FDv2ClientTest, IdentifyDoesNotSendThePreviousContextsBasis) {
    namespace http = boost::beast::http;

    std::mutex mutex;
    std::vector<std::string> targets;
    std::function<bool(std::string const&)> send_to_first;
    async::Promise<std::monostate> second_connected;
    auto second_connected_future = second_connected.GetFuture();
    async::Promise<std::monostate> applied;
    auto applied_future = applied.GetFuture();
    std::once_flag pushed_once;
    bool late_update_applied = false;

    sse::test::MockSSEServer server;
    server.start([&](http::request<http::string_body> const& req,
                     std::function<void(http::response<http::string_body>)>
                         send_response,
                     std::function<bool(std::string const&)> send_sse_event,
                     std::function<void()> /* close_connection */) {
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "text/event-stream");
        res.chunked(true);
        send_response(res);

        std::lock_guard lock{mutex};
        // Record the target of each request.
        targets.emplace_back(req.target());
        if (targets.size() == 1) {
            send_to_first = send_sse_event;
            // Send a full data set on the first stream.
            send_sse_event(
                "event: server-intent\n"
                R"(data: {"payloads":[{"id":"p1","target":1,)"
                R"("intentCode":"xfer-full"}]})"
                "\n\n"
                "event: put-object\n"
                R"(data: {"version":1,"kind":"flag-eval","key":"flagA",)"
                R"("object":{"value":"a-1","variation":0}})"
                "\n\n"
                "event: payload-transferred\n"
                R"(data: {"state":"a-1","version":1})"
                "\n\n");
        } else if (targets.size() == 2) {
            second_connected.Resolve(std::monostate{});
        }
    });

    auto builder = ConfigBuilder("the-key");
    builder.DataSource().Method(
        DataSourceBuilder::FDv2()
            .InitialMode(ConnectionMode::kStreaming)
            .CustomizeMode(ConnectionMode::kStreaming,
                           FDv2Builder::Mode()
                               .Initializer(FDv2Builder::Cache())
                               .Synchronizer(FDv2Builder::Streaming().BaseUrl(
                                   server.url()))));
    builder.Events().Disable();
    builder.Persistence().None();

    Client client(builder.Build().value(),
                  ContextBuilder().Kind("user", "first").Build());
    auto started = client.StartAsync();
    ASSERT_EQ(std::future_status::ready, started.wait_for(5s));
    ASSERT_TRUE(started.get());

    // A change to flagA shows that the SDK applied the update.
    auto flag_connection = client.FlagNotifier().OnFlagChange(
        "flagA", [&](std::shared_ptr<flag_manager::FlagValueChangeEvent>) {
            applied.Resolve(std::monostate{});
        });

    auto status_connection = client.DataSourceStatus().OnDataSourceStatusChange(
        [&](data_sources::DataSourceStatus status) {
            // Identify sets this status before the old source closes.
            if (status.State() != data_sources::DataSourceStatus::
                                      DataSourceState::kInitializing) {
                return;
            }
            std::call_once(pushed_once, [&] {
                std::function<bool(std::string const&)> send;
                {
                    std::lock_guard lock{mutex};
                    send = send_to_first;
                }
                // Send an update on the old stream.
                send(
                    "event: put-object\n"
                    R"(data: {"version":2,"kind":"flag-eval","key":"flagA",)"
                    R"("object":{"value":"a-2","variation":1}})"
                    "\n\n"
                    "event: payload-transferred\n"
                    R"(data: {"state":"a-2","version":2})"
                    "\n\n");
                // Wait until the SDK applies the update.
                late_update_applied =
                    applied_future.WaitForResult(5s).has_value();
            });
        });

    // Identify the second context.
    client.IdentifyAsync(ContextBuilder().Kind("user", "second").Build());

    // The SDK applied the update from the old stream.
    ASSERT_TRUE(late_update_applied);
    // The SDK opened a stream for the second context.
    ASSERT_TRUE(second_connected_future.WaitForResult(5s).has_value());
    std::lock_guard lock{mutex};
    // The first request for the second context has no basis.
    EXPECT_EQ(std::string::npos, targets[1].find("basis="))
        << "The second context requested " << targets[1];
}

// A customized mode that sets no FDv1 fallback still falls back to FDv1.
TEST(FDv2ClientTest, CustomizedModeKeepsTheBuiltInFDv1Fallback) {
    namespace http = boost::beast::http;

    async::Promise<std::monostate> polled;
    auto polled_future = polled.GetFuture();

    sse::test::MockSSEServer server;
    server.start([&](http::request<http::string_body> const& req,
                     std::function<void(http::response<http::string_body>)>
                         send_response,
                     std::function<bool(std::string const&)> send_sse_event,
                     std::function<void()> /* close_connection */) {
        std::string const target(req.target());
        // Answer FDv1 polls with no flags.
        if (target.rfind("/msdk/evalx/contexts", 0) == 0) {
            http::response<http::string_body> res{http::status::ok,
                                                  req.version()};
            res.set(http::field::content_type, "application/json");
            res.body() = "{}";
            res.prepare_payload();
            send_response(res);
            // Record the FDv1 poll.
            polled.Resolve(std::monostate{});
            return;
        }

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "text/event-stream");
        res.chunked(true);
        send_response(res);
        // Direct the SDK to FDv1.
        send_sse_event(
            "event: goodbye\n"
            R"(data: {"reason":"fallback","protocolFallbackTTL":60})"
            "\n\n");
    });

    auto builder = ConfigBuilder("the-key");
    builder.ServiceEndpoints()
        .PollingBaseUrl(server.url())
        .StreamingBaseUrl(server.url())
        .EventsBaseUrl(server.url());
    builder.DataSource().Method(
        DataSourceBuilder::FDv2()
            .InitialMode(ConnectionMode::kStreaming)
            .CustomizeMode(
                ConnectionMode::kStreaming,
                FDv2Builder::Mode().Synchronizer(FDv2Builder::Streaming())));
    builder.Events().Disable();
    builder.Persistence().None();

    Client client(builder.Build().value(),
                  ContextBuilder().Kind("user", "user-key").Build());

    // Start the SDK in the customized streaming mode.
    client.StartAsync();

    // The SDK polls the FDv1 endpoint.
    EXPECT_TRUE(polled_future.WaitForResult(5s).has_value());
}
