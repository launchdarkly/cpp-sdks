// The Beast backend sends HttpRequest::Path() verbatim as the request target.
// These tests put percent-encoded URLs on the wire and check what a real HTTP
// parser at the other end sees.
#ifndef LD_CURL_NETWORKING

#include <gtest/gtest.h>

#include <launchdarkly/config/shared/builders/http_properties_builder.hpp>
#include <launchdarkly/config/shared/sdks.hpp>
#include <launchdarkly/network/asio_requester.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <chrono>
#include <memory>
#include <optional>
#include <string>

using launchdarkly::config::shared::ClientSDK;
using launchdarkly::config::shared::builders::HttpPropertiesBuilder;
using launchdarkly::config::shared::built::HttpProperties;
using launchdarkly::network::AsioRequester;
using launchdarkly::network::HttpMethod;
using launchdarkly::network::HttpRequest;
using launchdarkly::network::HttpResult;
using namespace std::chrono_literals;

namespace {

HttpProperties Props() {
    return HttpPropertiesBuilder<ClientSDK>()
        .ConnectTimeout(2s)
        .ResponseTimeout(2s)
        .Build();
}

// Accepts one connection on an ephemeral loopback port, records the request
// as Beast parsed it, answers 200, and closes the socket.
class OneShotServer : public std::enable_shared_from_this<OneShotServer> {
   public:
    explicit OneShotServer(net::io_context& ioc)
        : acceptor_(ioc, tcp::endpoint(net::ip::make_address("127.0.0.1"), 0)),
          socket_(ioc) {
        response_.result(http::status::ok);
        response_.set(http::field::content_type, "application/json");
        response_.body() = "{}";
        response_.prepare_payload();
    }

    unsigned short Port() const { return acceptor_.local_endpoint().port(); }

    void Start() {
        acceptor_.async_accept(
            socket_,
            [self = shared_from_this()](boost::system::error_code const& ec) {
                boost::system::error_code ignored;
                self->acceptor_.close(ignored);
                if (ec) {
                    return;
                }
                http::async_read(
                    self->socket_, self->buffer_, self->request_,
                    [self](boost::system::error_code const& ec, std::size_t) {
                        if (ec) {
                            self->read_error_ = ec.message();
                            self->Close();
                            return;
                        }
                        self->seen_ = self->request_;
                        http::async_write(
                            self->socket_, self->response_,
                            [self](boost::system::error_code const&,
                                   std::size_t) { self->Close(); });
                    });
            });
    }

    std::optional<http::request<http::string_body>> const& Seen() const {
        return seen_;
    }

    std::optional<std::string> const& ReadError() const { return read_error_; }

   private:
    void Close() {
        boost::system::error_code ignored;
        socket_.shutdown(tcp::socket::shutdown_both, ignored);
        socket_.close(ignored);
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> request_;
    http::response<http::string_body> response_;
    std::optional<http::request<http::string_body>> seen_;
    std::optional<std::string> read_error_;
};

// Sends one GET for the given target through the asio requester and returns
// the request exactly as the server parsed it.
std::optional<http::request<http::string_body>> RoundTrip(
    std::string const& target) {
    net::io_context ioc;
    auto server = std::make_shared<OneShotServer>(ioc);
    server->Start();

    auto props = Props();
    AsioRequester requester(ioc.get_executor(), props.Tls());
    std::optional<HttpResult> result;
    requester.Request(HttpRequest("http://127.0.0.1:" +
                                      std::to_string(server->Port()) + target,
                                  HttpMethod::kGet, props, std::nullopt),
                      [&](HttpResult res) { result = std::move(res); });
    ioc.run_for(5s);

    EXPECT_FALSE(server->ReadError().has_value())
        << "server could not parse the request: " << *server->ReadError();
    EXPECT_TRUE(result.has_value());
    if (result) {
        EXPECT_FALSE(result->IsError()) << *result;
        EXPECT_EQ(200u, result->Status());
    }
    return server->Seen();
}

}  // namespace

// A server-supplied value that a URL builder percent-encoded, such as the
// FDv2 "basis" selector state. Decoding it again on the way out would end
// the request line at the CR LF and turn the remainder into a header.
TEST(AsioRequesterTest, PercentEncodedQueryReachesTheServerIntact) {
    std::string const target =
        "/sdk/poll/eval?basis=x%0D%0AX-Injected:%201&f=a%26b%20c%23d";

    auto seen = RoundTrip(target);

    ASSERT_TRUE(seen.has_value());
    EXPECT_EQ(target, seen->target());
    EXPECT_EQ(seen->end(), seen->find("X-Injected"));
}

TEST(AsioRequesterTest, PercentEncodedPathReachesTheServerIntact) {
    std::string const target = "/ld%20relay/p%2Fq/sdk/latest-all";

    auto seen = RoundTrip(target);

    ASSERT_TRUE(seen.has_value());
    EXPECT_EQ(target, seen->target());
}

#endif  // LD_CURL_NETWORKING
