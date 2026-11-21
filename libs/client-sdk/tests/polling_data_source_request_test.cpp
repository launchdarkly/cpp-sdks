// The FDv1 polling source builds its request URL from the configured base
// URL. This test sends a request from the source to a loopback server and
// checks the target it parsed, so a base URL that already carries a query
// keeps it and withReasons is joined with '&'.
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>

#include <launchdarkly/config/shared/defaults.hpp>
#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/encoding/base_64.hpp>
#include <launchdarkly/logging/null_logger.hpp>
#include <launchdarkly/serialization/json_context.hpp>

#include "data_sources/data_source_status_manager.hpp"
#include "data_sources/polling_data_source.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

using namespace launchdarkly;
using namespace launchdarkly::client_side;
using namespace launchdarkly::client_side::data_sources;
using namespace std::chrono_literals;

namespace {

class NullSink : public IDataSourceUpdateSink {
   public:
    void Init(Context const&,
              std::unordered_map<std::string, ItemDescriptor>) override {}
    void Upsert(Context const&, std::string, ItemDescriptor) override {}
    void Apply(Context const&, FlagChangeSet, bool) override {}
};

// Accepts one connection on an ephemeral loopback port, records the target
// as Beast parsed it, answers 200, then stops the io_context so the test does
// not wait for the source's next poll.
class OneShotServer : public std::enable_shared_from_this<OneShotServer> {
   public:
    explicit OneShotServer(net::io_context& ioc)
        : ioc_(ioc),
          acceptor_(ioc, tcp::endpoint(net::ip::make_address("127.0.0.1"), 0)),
          socket_(ioc) {}

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
                                            "application/json");
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
    std::optional<std::string> target_;
};

}  // namespace

TEST(PollingDataSourceRequestTest, WithReasonsJoinsAnExistingQuery) {
    net::io_context ioc;
    auto server = std::make_shared<OneShotServer>(ioc);
    server->Start();

    auto const base = server->BaseUrl();
    config::shared::built::ServiceEndpoints const endpoints(base, base, base);
    config::shared::built::DataSourceConfig<
        config::shared::ClientSDK> const data_source_config{
        config::shared::Defaults<config::shared::ClientSDK>::PollingConfig(),
        /* with_reasons= */ true, /* use_report= */ false};
    auto const http_properties =
        config::shared::Defaults<config::shared::ClientSDK>::HttpProperties();

    auto const context = ContextBuilder().Kind("user", "user-key").Build();
    auto const encoded_context = encoding::Base64UrlEncode(
        boost::json::serialize(boost::json::value_from(context)));

    NullSink sink;
    DataSourceStatusManager status_manager;
    Logger logger = logging::NullLogger();

    auto source = std::make_shared<PollingDataSource>(
        endpoints, data_source_config, http_properties, ioc.get_executor(),
        context, sink, status_manager, logger);
    source->Start();
    ioc.run_for(5s);
    source->ShutdownAsync(nullptr);

    ASSERT_TRUE(server->Target().has_value())
        << "no request reached the server";
    EXPECT_EQ("/relay/msdk/evalx/contexts/" + encoded_context +
                  "?tok=a%26b&withReasons=true",
              *server->Target());
}
