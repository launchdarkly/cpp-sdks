#ifdef LD_CURL_NETWORKING

#include <gtest/gtest.h>

#include <launchdarkly/config/shared/builders/http_properties_builder.hpp>
#include <launchdarkly/config/shared/sdks.hpp>
#include <launchdarkly/network/curl_requester.hpp>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <chrono>
#include <memory>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

using launchdarkly::config::shared::ClientSDK;
using launchdarkly::config::shared::builders::HttpPropertiesBuilder;
using launchdarkly::network::CurlRequester;
using launchdarkly::network::HttpMethod;
using launchdarkly::network::HttpRequest;
using launchdarkly::network::HttpResult;

// Simple HTTP server for testing
class TestHttpServer {
   public:
    TestHttpServer(net::io_context& ioc, const unsigned short port)
        : acceptor_(ioc, tcp::endpoint(tcp::v4(), port)),
          socket_(ioc) {
        port_ = acceptor_.local_endpoint().port();
    }

    unsigned short port() const { return port_; }

    void Accept() {
        acceptor_.async_accept(socket_, [this](const beast::error_code& ec) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket_), handler_)
                    ->Run();
            }
            Accept();
        });
    }

    void SetHandler(
        std::function<http::response<http::string_body>(
            http::request<http::string_body> const&)> handler) {
        handler_ = std::move(handler);
    }

   private:
    class Session : public std::enable_shared_from_this<Session> {
       public:
        Session(tcp::socket socket,
                std::function<http::response<http::string_body>(
                    http::request<http::string_body> const&)> handler)
            : socket_(std::move(socket)), handler_(std::move(handler)) {}

        void Run() {
            http::async_read(
                socket_, buffer_, req_,
                [self = shared_from_this()](const beast::error_code& ec,
                                            std::size_t bytes_transferred) {
                    boost::ignore_unused(bytes_transferred);
                    if (!ec) {
                        self->HandleRequest();
                    }
                });
        }

       private:
        void HandleRequest() {
            res_ = handler_(req_);
            res_.prepare_payload();

            http::async_write(socket_, res_,
                              [self = shared_from_this()](
                                  beast::error_code ec, std::size_t) {
                                  self->socket_.shutdown(
                                      tcp::socket::shutdown_send, ec);
                              });
        }

        tcp::socket socket_;
        beast::flat_buffer buffer_;
        http::request<http::string_body> req_;
        http::response<http::string_body> res_;
        std::function<http::response<http::string_body>(
            http::request<http::string_body> const&)>
            handler_;
    };

    tcp::acceptor acceptor_;
    tcp::socket socket_;
    unsigned short port_;
    std::function<http::response<http::string_body>(
        http::request<http::string_body> const&)>
        handler_;
};

class CurlRequesterTest : public ::testing::Test {
   protected:
    void SetUp() override {
        server_ = std::make_unique<TestHttpServer>(ioc_, 0);
        server_->Accept();

        // Run io_context in a separate thread
        thread_ = std::thread([this]() { ioc_.run(); });

        // Give the server time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        ioc_.stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    std::string GetServerUrl(std::string const& path = "/") const {
        return "http://127.0.0.1:" + std::to_string(server_->port()) + path;
    }

    net::io_context ioc_;
    std::unique_ptr<TestHttpServer> server_;
    std::thread thread_;
};

TEST_F(CurlRequesterTest, CanMakeBasicGetRequest) {
    server_->SetHandler(
        [](http::request<http::string_body> const& req)
            -> http::response<http::string_body> {
            EXPECT_EQ(http::verb::get, req.method());
            EXPECT_EQ("/test", req.target());

            http::response<http::string_body> res{http::status::ok,
                                                   req.version()};
            res.set(http::field::content_type, "text/plain");
            res.body() = "Hello, World!";
            return res;
        });

    net::io_context client_ioc;
    CurlRequester requester(
        client_ioc.get_executor(),
        launchdarkly::config::shared::built::TlsOptions());

    HttpRequest request(GetServerUrl("/test"), HttpMethod::kGet,
                        HttpPropertiesBuilder<ClientSDK>().Build(),
                        std::nullopt);

    bool callback_called = false;
    HttpResult result(std::nullopt);

    requester.Request(
        std::move(request),
        [&callback_called, &result, &client_ioc](HttpResult const& res) {
            callback_called = true;
            result = res;
            client_ioc.stop();
        });

    client_ioc.run();

    ASSERT_TRUE(callback_called);
    EXPECT_FALSE(result.IsError());
    EXPECT_EQ(200, result.Status());
    EXPECT_EQ("Hello, World!", result.Body().value());
}

TEST_F(CurlRequesterTest, CanMakePostRequestWithBody) {
    server_->SetHandler(
        [](http::request<http::string_body> const& req)
            -> http::response<http::string_body> {
            EXPECT_EQ(http::verb::post, req.method());
            EXPECT_EQ("/echo", req.target());
            EXPECT_EQ("test data", req.body());

            http::response<http::string_body> res{http::status::ok,
                                                   req.version()};
            res.set(http::field::content_type, "text/plain");
            res.body() = "Received: " + std::string(req.body());
            return res;
        });

    net::io_context client_ioc;
    CurlRequester requester(
        client_ioc.get_executor(),
        launchdarkly::config::shared::built::TlsOptions());

    HttpRequest request(GetServerUrl("/echo"), HttpMethod::kPost,
                        HttpPropertiesBuilder<ClientSDK>().Build(),
                        "test data");

    bool callback_called = false;
    HttpResult result(std::nullopt);

    requester.Request(
        std::move(request),
        [&callback_called, &result, &client_ioc](HttpResult const& res) {
            callback_called = true;
            result = res;
            client_ioc.stop();
        });

    client_ioc.run();

    ASSERT_TRUE(callback_called);
    EXPECT_FALSE(result.IsError());
    EXPECT_EQ(200, result.Status());
    EXPECT_EQ("Received: test data", result.Body().value());
}

TEST_F(CurlRequesterTest, HandlesCustomHeaders) {
    server_->SetHandler(
        [](http::request<http::string_body> const& req)
            -> http::response<http::string_body> {
            const auto header_it = req.find("X-Custom-Header");
            EXPECT_NE(req.end(), header_it);
            if (header_it != req.end()) {
                EXPECT_EQ("custom-value", header_it->value());
            }

            http::response<http::string_body> res{http::status::ok,
                                                   req.version()};
            res.set("X-Response-Header", "response-value");
            res.body() = "OK";
            return res;
        });

    net::io_context client_ioc;
    CurlRequester requester(
        client_ioc.get_executor(),
        launchdarkly::config::shared::built::TlsOptions());

    auto properties = HttpPropertiesBuilder<ClientSDK>()
                          .Header("X-Custom-Header", "custom-value")
                          .Build();

    HttpRequest request(GetServerUrl("/headers"), HttpMethod::kGet,
                        std::move(properties), std::nullopt);

    bool callback_called = false;
    HttpResult result(std::nullopt);

    requester.Request(
        std::move(request),
        [&callback_called, &result, &client_ioc](HttpResult const& res) {
            callback_called = true;
            result = res;
            client_ioc.stop();
        });

    client_ioc.run();

    ASSERT_TRUE(callback_called);
    EXPECT_FALSE(result.IsError());
    EXPECT_EQ(200, result.Status());
    EXPECT_EQ(1, result.Headers().count("X-Response-Header"));
    EXPECT_EQ("response-value", result.Headers().at("X-Response-Header"));
}

TEST_F(CurlRequesterTest, Handles404Status) {
    server_->SetHandler([](http::request<http::string_body> const& req)
                            -> http::response<http::string_body> {
        http::response<http::string_body> res{http::status::not_found,
                                               req.version()};
        res.body() = "Not Found";
        return res;
    });

    net::io_context client_ioc;
    const CurlRequester requester(
        client_ioc.get_executor(),
        launchdarkly::config::shared::built::TlsOptions());

    HttpRequest request(GetServerUrl("/notfound"), HttpMethod::kGet,
                        HttpPropertiesBuilder<ClientSDK>().Build(),
                        std::nullopt);

    bool callback_called = false;
    HttpResult result(std::nullopt);

    requester.Request(
        std::move(request),
        [&callback_called, &result, &client_ioc](HttpResult const& res) {
            callback_called = true;
            result = res;
            client_ioc.stop();
        });

    client_ioc.run();

    ASSERT_TRUE(callback_called);
    EXPECT_FALSE(result.IsError());
    EXPECT_EQ(404, result.Status());
    EXPECT_EQ("Not Found", result.Body().value());
}

TEST_F(CurlRequesterTest, HandlesInvalidUrl) {
    net::io_context client_ioc;
    const CurlRequester requester(
    client_ioc.get_executor(),
        launchdarkly::config::shared::built::TlsOptions());

    HttpRequest request("not a valid url", HttpMethod::kGet,
                        HttpPropertiesBuilder<ClientSDK>().Build(),
                        std::nullopt);

    bool callback_called = false;
    HttpResult result(std::nullopt);

    requester.Request(
        std::move(request),
        [&callback_called, &result, &client_ioc](HttpResult const& res) {
            callback_called = true;
            result = res;
            client_ioc.stop();
        });

    client_ioc.run();

    ASSERT_TRUE(callback_called);
    EXPECT_TRUE(result.IsError());
}

#endif  // LD_CURL_NETWORKING
