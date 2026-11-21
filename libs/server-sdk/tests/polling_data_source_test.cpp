#include <gtest/gtest.h>

#include <data_components/status_notifications/data_source_status_manager.hpp>
#include <data_interfaces/destination/idestination.hpp>
#include <data_systems/background_sync/sources/polling/polling_data_source.hpp>

#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/server_side/config/builders/all_builders.hpp>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>

using namespace launchdarkly;
using namespace launchdarkly::server_side::data_components;
using namespace launchdarkly::server_side::data_interfaces;
using namespace launchdarkly::server_side::data_systems;
using namespace std::chrono_literals;

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

namespace {

Logger MakeNullLogger() {
    struct NullBackend : ILogBackend {
        bool Enabled(LogLevel) noexcept override { return false; }
        void Write(LogLevel, std::string) noexcept override {}
    };
    return Logger{std::make_shared<NullBackend>()};
}

class NoopDestination final : public IDestination {
   public:
    void Init(data_model::SDKDataSet) override {}
    void Upsert(std::string const&, data_model::FlagDescriptor) override {}
    void Upsert(std::string const&, data_model::SegmentDescriptor) override {}
    [[nodiscard]] std::string const& Identity() const override {
        static std::string const identity = "noop destination";
        return identity;
    }
};

// Answers every request with HTTP 200 and a fixed body, and counts the
// requests.
//
// Thread safety: the accept and session handlers run on the io_context passed
// to the constructor. WaitForRequests is safe from any thread; the request
// count is protected by mutex_.
class PollServer {
   public:
    PollServer(boost::asio::io_context& ioc, std::string body)
        : acceptor_(ioc,
                    tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"),
                                  /* port= */ 0)),
          body_(std::move(body)) {
        Accept();
    }

    [[nodiscard]] std::string Url() const {
        return "http://127.0.0.1:" +
               std::to_string(acceptor_.local_endpoint().port());
    }

    // Returns true when the server has received at least count requests
    // before the timeout.
    bool WaitForRequests(int count, std::chrono::milliseconds timeout) {
        std::unique_lock lock(mutex_);
        return cv_.wait_for(lock, timeout,
                            [this, count] { return requests_ >= count; });
    }

   private:
    struct Session : std::enable_shared_from_this<Session> {
        Session(tcp::socket socket, PollServer* server)
            : socket(std::move(socket)), server(server) {}

        void Run() {
            http::async_read(socket, buffer, request,
                             [self = shared_from_this()](
                                 beast::error_code const& ec, std::size_t) {
                                 if (ec) {
                                     return;
                                 }
                                 self->server->CountRequest();
                                 self->response.result(http::status::ok);
                                 self->response.set(http::field::content_type,
                                                    "application/json");
                                 self->response.body() = self->server->body_;
                                 self->response.keep_alive(false);
                                 self->response.prepare_payload();
                                 http::async_write(
                                     self->socket, self->response,
                                     [self](beast::error_code ec, std::size_t) {
                                         self->socket.shutdown(
                                             tcp::socket::shutdown_send, ec);
                                     });
                             });
        }

        tcp::socket socket;
        beast::flat_buffer buffer;
        http::request<http::string_body> request;
        http::response<http::string_body> response;
        PollServer* server;
    };

    void Accept() {
        acceptor_.async_accept(
            [this](beast::error_code const& ec, tcp::socket socket) {
                if (ec) {
                    return;
                }
                std::make_shared<Session>(std::move(socket), this)->Run();
                Accept();
            });
    }

    void CountRequest() {
        std::lock_guard lock(mutex_);
        ++requests_;
        cv_.notify_all();
    }

    tcp::acceptor acceptor_;
    std::string const body_;

    // Protected by mutex_.
    std::mutex mutex_;
    std::condition_variable cv_;
    int requests_ = 0;
};

struct PollBody {
    std::string name;
    std::string body;
};

}  // namespace

class PollingDataSourceNextPollTest
    : public ::testing::TestWithParam<PollBody> {};

TEST_P(PollingDataSourceNextPollTest, PollsAgainAfterA200Response) {
    boost::asio::io_context ioc;
    auto work = boost::asio::make_work_guard(ioc);
    PollServer server(ioc, GetParam().body);

    server_side::config::built::BackgroundSyncConfig::PollingConfig const
        polling_config{
            /* poll_interval= */ 0s, /* polling_get_path= */ "/sdk/latest-all",
            /* min_polling_interval= */ 0s, /* filter_key= */ std::nullopt};
    auto const endpoints = server_side::config::builders::EndpointsBuilder()
                               .RelayProxyBaseURL(server.Url())
                               .Build();
    ASSERT_TRUE(endpoints);
    auto const http_properties =
        server_side::config::builders::HttpPropertiesBuilder().Build();

    auto const logger = MakeNullLogger();
    DataSourceStatusManager status_manager;
    NoopDestination destination;
    auto source = std::make_shared<PollingDataSource>(
        ioc.get_executor(), logger, status_manager, *endpoints, polling_config,
        http_properties);

    // Start the data source. It sends the first poll at once.
    source->StartAsync(&destination, /* bootstrap_data= */ nullptr);
    std::thread thread([&ioc] { ioc.run(); });

    // The first poll gets a 200 response. The data source must poll again.
    EXPECT_TRUE(server.WaitForRequests(2, 5s));

    work.reset();
    ioc.stop();
    thread.join();
}

INSTANTIATE_TEST_SUITE_P(
    PollResponses,
    PollingDataSourceNextPollTest,
    ::testing::Values(PollBody{"ValidData", R"({"flags":{},"segments":{}})"},
                      PollBody{"MalformedJson", R"({"flags":)"},
                      PollBody{"SchemaMismatch",
                               R"({"flags":5,"segments":{}})"}),
    [](::testing::TestParamInfo<PollBody> const& info) {
        return info.param.name;
    });
