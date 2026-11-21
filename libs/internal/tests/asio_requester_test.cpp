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
#include <vector>

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

// Answers the first request with a 301 to the given location and every
// later request with 200. Records the target of each request as Beast parsed
// it, and stops accepting after the expected number of requests.
class RedirectServer : public std::enable_shared_from_this<RedirectServer> {
   public:
    RedirectServer(net::io_context& ioc, std::size_t expected_requests)
        : acceptor_(ioc, tcp::endpoint(net::ip::make_address("127.0.0.1"), 0)),
          socket_(ioc),
          expected_requests_(expected_requests) {}

    unsigned short Port() const { return acceptor_.local_endpoint().port(); }

    // The location is given here rather than in the constructor, so the
    // caller can put the server's own port into it.
    void Start(std::string location) {
        location_ = std::move(location);
        Accept();
    }

    std::vector<std::string> const& Targets() const { return targets_; }

    std::optional<std::string> const& ReadError() const { return read_error_; }

   private:
    void Accept() {
        acceptor_.async_accept(
            socket_,
            [self = shared_from_this()](boost::system::error_code const& ec) {
                if (ec) {
                    return;
                }
                self->buffer_.consume(self->buffer_.size());
                self->request_ = {};
                http::async_read(
                    self->socket_, self->buffer_, self->request_,
                    [self](boost::system::error_code const& ec, std::size_t) {
                        if (ec) {
                            self->read_error_ = ec.message();
                            self->Finish();
                            return;
                        }
                        self->targets_.emplace_back(self->request_.target());
                        self->response_ = {};
                        if (self->targets_.size() == 1) {
                            self->response_.result(
                                http::status::moved_permanently);
                            self->response_.set(http::field::location,
                                                self->location_);
                        } else {
                            self->response_.result(http::status::ok);
                            self->response_.body() = "{}";
                        }
                        self->response_.prepare_payload();
                        http::async_write(
                            self->socket_, self->response_,
                            [self](boost::system::error_code const&,
                                   std::size_t) { self->Finish(); });
                    });
            });
    }

    void Finish() {
        boost::system::error_code ignored;
        socket_.shutdown(tcp::socket::shutdown_both, ignored);
        socket_.close(ignored);
        if (targets_.size() < expected_requests_ && !read_error_) {
            Accept();
            return;
        }
        acceptor_.close(ignored);
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> request_;
    http::response<http::string_body> response_;
    std::string location_;
    std::size_t expected_requests_;
    std::vector<std::string> targets_;
    std::optional<std::string> read_error_;
};

struct RedirectOutcome {
    std::vector<std::string> targets;
    std::optional<HttpResult> result;
};

// Sends one GET for the given target. The server answers with a 301 to the
// location, in which "{port}" is replaced with the server port. Returns the
// targets the server parsed and the result the requester delivered.
RedirectOutcome FollowRedirect(std::string const& target,
                               std::string const& location,
                               std::size_t expected_requests) {
    net::io_context ioc;
    auto server = std::make_shared<RedirectServer>(ioc, expected_requests);
    std::string resolved_location = location;
    if (auto const pos = resolved_location.find("{port}");
        pos != std::string::npos) {
        resolved_location.replace(pos, 6, std::to_string(server->Port()));
    }
    server->Start(resolved_location);

    auto props = Props();
    AsioRequester requester(ioc.get_executor(), props.Tls());
    RedirectOutcome outcome;
    requester.Request(HttpRequest("http://127.0.0.1:" +
                                      std::to_string(server->Port()) + target,
                                  HttpMethod::kGet, props, std::nullopt),
                      [&](HttpResult res) { outcome.result = std::move(res); });
    ioc.run_for(5s);

    EXPECT_FALSE(server->ReadError().has_value())
        << "server could not parse a request: " << *server->ReadError();
    outcome.targets = server->Targets();
    return outcome;
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

// Path normalization may decode escapes of characters that are legal in a
// path (older Boost releases include %2F), so only characters that cannot
// appear raw in a request target are checked here.
TEST(AsioRequesterTest, PercentEncodedPathReachesTheServerIntact) {
    std::string const target = "/ld%20relay/p%23q/sdk/latest-all";

    auto seen = RoundTrip(target);

    ASSERT_TRUE(seen.has_value());
    EXPECT_EQ(target, seen->target());
}

// A relative Location is resolved against the URL of the redirected request.
// Its percent-encoding is server-supplied and reaches the server unchanged.
TEST(AsioRequesterTest, RelativeRedirectLocationIsResolvedAgainstTheRequest) {
    auto outcome = FollowRedirect("/orig/path?keep=1", "/new%20path?x=%26y", 2);

    ASSERT_EQ(2u, outcome.targets.size());
    EXPECT_EQ("/orig/path?keep=1", outcome.targets[0]);
    EXPECT_EQ("/new%20path?x=%26y", outcome.targets[1]);
    ASSERT_TRUE(outcome.result.has_value());
    EXPECT_FALSE(outcome.result->IsError()) << *outcome.result;
    EXPECT_EQ(200u, outcome.result->Status());
}

TEST(AsioRequesterTest,
     RelativeRedirectLocationWithoutASlashKeepsTheParentPath) {
    auto outcome = FollowRedirect("/orig/path", "sibling", 2);

    ASSERT_EQ(2u, outcome.targets.size());
    EXPECT_EQ("/orig/sibling", outcome.targets[1]);
}

TEST(AsioRequesterTest, AbsoluteRedirectLocationIsFollowed) {
    auto outcome = FollowRedirect(
        "/orig", "http://127.0.0.1:{port}/abs%20path?basis=%0D%0A", 2);

    ASSERT_EQ(2u, outcome.targets.size());
    EXPECT_EQ("/abs%20path?basis=%0D%0A", outcome.targets[1]);
    ASSERT_TRUE(outcome.result.has_value());
    EXPECT_EQ(200u, outcome.result->Status());
}

// A Location that is not a valid URI reference cannot be resolved, so the
// request fails instead of sending a guessed target.
TEST(AsioRequesterTest, MalformedRedirectLocationFailsTheRequest) {
    auto outcome = FollowRedirect("/orig", "/bad%zz", 1);

    ASSERT_EQ(1u, outcome.targets.size());
    ASSERT_TRUE(outcome.result.has_value());
    EXPECT_TRUE(outcome.result->IsError());
}

#endif  // LD_CURL_NETWORKING
