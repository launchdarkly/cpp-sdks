// The FDv1 polling and streaming sources build their request URL from the
// configured base URL. These tests send a request from each source to a
// loopback server and check the target it parsed, so a base URL that already
// carries a query keeps it and the SDK's own parameters are joined with '&'.
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <launchdarkly/config/shared/defaults.hpp>
#include <launchdarkly/logging/null_logger.hpp>
#include <launchdarkly/server_side/config/built/all_built.hpp>

#include <data_components/status_notifications/data_source_status_manager.hpp>
#include <data_interfaces/destination/idestination.hpp>
#include <data_systems/background_sync/sources/polling/polling_data_source.hpp>
#include <data_systems/background_sync/sources/streaming/streaming_data_source.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

using namespace launchdarkly;
using namespace launchdarkly::server_side;
using namespace launchdarkly::server_side::data_systems;
using namespace std::chrono_literals;

namespace {

class NullDestination : public data_interfaces::IDestination {
   public:
    void Init(data_model::SDKDataSet) override {}
    void Upsert(std::string const&, data_model::FlagDescriptor) override {}
    void Upsert(std::string const&, data_model::SegmentDescriptor) override {}
    std::string const& Identity() const override {
        static std::string const identity = "null";
        return identity;
    }
};

// Accepts one connection on an ephemeral loopback port, records the target
// as Beast parsed it, answers 200 with the given content type, then stops
// the io_context so the test does not wait for the source's next poll.
class OneShotServer : public std::enable_shared_from_this<OneShotServer> {
   public:
    OneShotServer(net::io_context& ioc, std::string content_type)
        : ioc_(ioc),
          acceptor_(ioc, tcp::endpoint(net::ip::make_address("127.0.0.1"), 0)),
          socket_(ioc),
          content_type_(std::move(content_type)) {}

    std::string BaseUrl() const {
        return "http://127.0.0.1:" +
               std::to_string(acceptor_.local_endpoint().port()) +
               "/relay?tok=a%26b";
    }

    void Start() {
        acceptor_.async_accept(
            socket_,
            [self = shared_from_this()](boost::system::error_code const& ec) {
                if (ec) {
                    return;
                }
                http::async_read(
                    self->socket_, self->buffer_, self->request_,
                    [self](boost::system::error_code const& ec, std::size_t) {
                        if (!ec) {
                            self->target_ =
                                std::string(self->request_.target());
                        }
                        self->response_.result(http::status::ok);
                        self->response_.set(http::field::content_type,
                                            self->content_type_);
                        self->response_.body() = "{}";
                        self->response_.prepare_payload();
                        http::async_write(
                            self->socket_, self->response_,
                            [self](boost::system::error_code const&,
                                   std::size_t) {
                                boost::system::error_code ignored;
                                self->socket_.shutdown(
                                    tcp::socket::shutdown_both, ignored);
                                self->socket_.close(ignored);
                                self->acceptor_.close(ignored);
                                self->ioc_.stop();
                            });
                    });
            });
    }

    std::optional<std::string> const& Target() const { return target_; }

   private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> request_;
    http::response<http::string_body> response_;
    std::string content_type_;
    std::optional<std::string> target_;
};

}  // namespace

TEST(Fdv1SourceRequestTest, PollingFilterJoinsAnExistingQuery) {
    net::io_context ioc;
    auto server = std::make_shared<OneShotServer>(ioc, "application/json");
    server->Start();

    auto const base = server->BaseUrl();
    server_side::config::built::ServiceEndpoints const endpoints(base, base,
                                                                 base);
    auto polling = launchdarkly::config::shared::Defaults<
        launchdarkly::config::shared::ServerSDK>::PollingConfig();
    polling.filter_key = "my-filter";
    auto const http_properties = launchdarkly::config::shared::Defaults<
        launchdarkly::config::shared::ServerSDK>::HttpProperties();

    Logger logger = logging::NullLogger();
    data_components::DataSourceStatusManager status_manager;
    NullDestination destination;

    auto source = std::make_shared<PollingDataSource>(
        ioc.get_executor(), logger, status_manager, endpoints, polling,
        http_properties);
    source->StartAsync(&destination, /* bootstrap_data= */ nullptr);
    ioc.run_for(5s);
    source->ShutdownAsync(nullptr);

    ASSERT_TRUE(server->Target().has_value())
        << "no request reached the server";
    EXPECT_EQ("/relay/sdk/latest-all?tok=a%26b&filter=my-filter",
              *server->Target());
}

TEST(Fdv1SourceRequestTest, StreamingFilterJoinsAnExistingQuery) {
    net::io_context ioc;
    auto server = std::make_shared<OneShotServer>(ioc, "text/event-stream");
    server->Start();

    auto const base = server->BaseUrl();
    server_side::config::built::ServiceEndpoints const endpoints(base, base,
                                                                 base);
    auto streaming = launchdarkly::config::shared::Defaults<
        launchdarkly::config::shared::ServerSDK>::StreamingConfig();
    streaming.filter_key = "my-filter";
    auto const http_properties = launchdarkly::config::shared::Defaults<
        launchdarkly::config::shared::ServerSDK>::HttpProperties();

    Logger logger = logging::NullLogger();
    data_components::DataSourceStatusManager status_manager;
    NullDestination destination;

    auto source = std::make_shared<StreamingDataSource>(
        ioc.get_executor(), logger, status_manager, endpoints, streaming,
        http_properties);
    source->StartAsync(&destination, /* bootstrap_data= */ nullptr);
    ioc.run_for(5s);

    // Let the stream client finish its shutdown before the io_context and the
    // source are destroyed.
    source->ShutdownAsync(nullptr);
    ioc.restart();
    ioc.run_for(2s);

    ASSERT_TRUE(server->Target().has_value())
        << "no request reached the server";
    EXPECT_EQ("/relay/all?tok=a%26b&filter=my-filter", *server->Target());
}
