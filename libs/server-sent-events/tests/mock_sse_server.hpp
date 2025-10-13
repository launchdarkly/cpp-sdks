#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <memory>
#include <string>
#include <sstream>
#include <functional>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <vector>

namespace launchdarkly::sse::test {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace ssl = boost::asio::ssl;

/**
 * Mock SSE server for testing. Supports both HTTP and HTTPS.
 * Can be configured to send various SSE payloads, error responses, timeouts, etc.
 */
class MockSSEServer {
public:
    using RequestHandler = std::function<void(
        http::request<http::string_body> const& req,
        std::function<void(http::response<http::string_body>)> send_response,
        std::function<bool(std::string const&)> send_sse_event,
        std::function<void()> close_connection
    )>;

    MockSSEServer()
        : ioc_(),
          acceptor_(ioc_),
          running_(false),
          port_(0) {
    }

    ~MockSSEServer() {
        stop();
    }

    /**
     * Start the server on a random available port.
     * Returns the port number.
     */
    uint16_t start(RequestHandler handler, bool use_ssl = false) {
        handler_ = std::move(handler);
        use_ssl_ = use_ssl;

        // Bind to port 0 to get a random available port
        tcp::endpoint endpoint(tcp::v4(), 0);
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(net::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();

        port_ = acceptor_.local_endpoint().port();
        running_ = true;

        // Start accepting connections in a background thread
        server_thread_ = std::thread([this]() {
            do_accept();
            ioc_.run();
        });

        return port_;
    }

    void stop() {
        if (!running_) {
            return;
        }

        running_ = false;

        // Close all active connections
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            for (auto& conn : active_connections_) {
                if (auto c = conn.lock()) {
                    c->force_close();
                }
            }
            active_connections_.clear();
        }

        boost::system::error_code ec;
        acceptor_.close(ec);

        ioc_.stop();

        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }

    uint16_t port() const { return port_; }

    std::string url() const {
        return (use_ssl_ ? "https://" : "http://") +
               std::string("localhost:") + std::to_string(port_);
    }

private:
    void do_accept() {
        if (!running_) {
            return;
        }

        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    handle_connection(std::move(socket));
                }

                if (running_) {
                    do_accept();
                }
            });
    }

    void handle_connection(tcp::socket socket) {
        auto conn = std::make_shared<Connection>(
            std::move(socket), handler_);

        // Track active connections
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            active_connections_.push_back(conn);
        }

        conn->start();
    }

    struct Connection : std::enable_shared_from_this<Connection> {
        tcp::socket socket_;
        beast::flat_buffer buffer_;
        http::request<http::string_body> req_;
        RequestHandler handler_;
        std::atomic<bool> closed_;

        Connection(tcp::socket socket, RequestHandler handler)
            : socket_(std::move(socket)),
              handler_(std::move(handler)),
              closed_(false) {
        }

        void start() {
            do_read();
        }

        void force_close() {
            closed_ = true;
            boost::system::error_code ec;
            socket_.shutdown(tcp::socket::shutdown_both, ec);
            socket_.close(ec);
        }

        void do_read() {
            auto self = shared_from_this();
            http::async_read(
                socket_,
                buffer_,
                req_,
                [self](boost::system::error_code ec, std::size_t) {
                    if (!ec) {
                        self->handle_request();
                    }
                });
        }

        void handle_request() {
            auto self = shared_from_this();

            auto send_response = [self](http::response<http::string_body> res) {
                if (self->closed_) {
                    return;
                }

                boost::system::error_code ec;

                // For error responses and redirects (with body or no SSE), write complete response
                if (res.result() != http::status::ok || !res.chunked()) {
                    http::write(self->socket_, res, ec);
                    return;
                }

                // For SSE (chunked OK responses), manually send headers to keep connection open
                std::ostringstream oss;
                oss << "HTTP/1.1 " << res.result_int() << " " << res.reason() << "\r\n";
                for (auto const& field : res) {
                    oss << field.name_string() << ": " << field.value() << "\r\n";
                }
                oss << "\r\n";  // End of headers

                std::string header_str = oss.str();
                net::write(self->socket_, net::buffer(header_str), ec);
            };

            auto send_sse_event = [self](std::string const& data) -> bool {
                if (self->closed_) {
                    return false;
                }

                boost::system::error_code ec;

                // Send as chunked encoding: size in hex + CRLF + data + CRLF
                std::ostringstream chunk;
                chunk << std::hex << data.size() << "\r\n" << data << "\r\n";
                std::string chunk_str = chunk.str();

                net::write(self->socket_, net::buffer(chunk_str), ec);

                // If write failed, mark connection as closed to prevent further writes
                if (ec) {
                    self->closed_ = true;
                    return false;
                }

                return true;
            };

            auto close_connection = [self]() {
                if (self->closed_) {
                    return;
                }
                self->closed_ = true;

                // Send final chunk terminator for chunked encoding
                boost::system::error_code ec;
                std::string final_chunk = "0\r\n\r\n";
                net::write(self->socket_, net::buffer(final_chunk), ec);

                self->socket_.shutdown(tcp::socket::shutdown_both, ec);
                self->socket_.close(ec);
            };

            handler_(req_, send_response, send_sse_event, close_connection);
        }
    };

    net::io_context ioc_;
    tcp::acceptor acceptor_;
    std::thread server_thread_;
    RequestHandler handler_;
    std::atomic<bool> running_;
    std::atomic<uint16_t> port_;
    bool use_ssl_;
    std::mutex connections_mutex_;
    std::vector<std::weak_ptr<Connection>> active_connections_;
};

/**
 * Helper to send SSE-formatted events
 */
class SSEFormatter {
public:
    static std::string event(std::string const& data,
                            std::string const& event_type = "",
                            std::string const& id = "") {
        std::string result;

        if (!id.empty()) {
            result += "id: " + id + "\n";
        }

        if (!event_type.empty()) {
            result += "event: " + event_type + "\n";
        }

        // Handle multi-line data
        if (data.empty()) {
            // Empty data still needs at least one data field
            result += "data: \n";
        } else {
            size_t pos = 0;
            size_t found;
            while ((found = data.find('\n', pos)) != std::string::npos) {
                result += "data: " + data.substr(pos, found - pos) + "\n";
                pos = found + 1;
            }
            if (pos < data.length()) {
                result += "data: " + data.substr(pos) + "\n";
            }
        }

        result += "\n";
        return result;
    }

    static std::string comment(std::string const& text) {
        return ": " + text + "\n";
    }
};

/**
 * Common test handlers for typical scenarios
 */
class TestHandlers {
public:
    /**
     * Send a simple SSE stream with one event then close
     */
    static MockSSEServer::RequestHandler simple_event(std::string data) {
        return [data = std::move(data)](
            auto const&, auto send_response, auto send_sse_event, auto close) {

            http::response<http::string_body> res{http::status::ok, 11};
            res.set(http::field::content_type, "text/event-stream");
            res.set(http::field::cache_control, "no-cache");
            res.keep_alive(true);
            res.chunked(true);

            send_response(res);
            if (!send_sse_event(SSEFormatter::event(data))) {
                return;  // Connection closed or error
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            close();
        };
    }

    /**
     * Send multiple events
     */
    static MockSSEServer::RequestHandler multiple_events(std::vector<std::string> events) {
        return [events = std::move(events)](
            auto const&, auto send_response, auto send_sse_event, auto close) {

            http::response<http::string_body> res{http::status::ok, 11};
            res.set(http::field::content_type, "text/event-stream");
            res.keep_alive(true);
            res.chunked(true);

            send_response(res);

            for (auto const& data : events) {
                if (!send_sse_event(SSEFormatter::event(data))) {
                    return;  // Connection closed or error
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            close();
        };
    }

    /**
     * Return an HTTP error status
     */
    static MockSSEServer::RequestHandler http_error(http::status status) {
        return [status](auto const&, auto send_response, auto, auto) {
            http::response<http::string_body> res{status, 11};
            res.body() = "Error";
            res.prepare_payload();
            send_response(res);
        };
    }

    /**
     * Send a redirect
     */
    static MockSSEServer::RequestHandler redirect(std::string location, http::status status = http::status::moved_permanently) {
        return [location = std::move(location), status](
            auto const&, auto send_response, auto, auto) {

            http::response<http::string_body> res{status, 11};
            res.set(http::field::location, location);
            send_response(res);
        };
    }

    /**
     * Never respond (to test timeouts)
     */
    static MockSSEServer::RequestHandler timeout() {
        return [](auto const&, auto, auto, auto) {
            // Do nothing - let the connection hang
            std::this_thread::sleep_for(std::chrono::hours(1));
        };
    }

    /**
     * Close connection immediately
     */
    static MockSSEServer::RequestHandler immediate_close() {
        return [](auto const&, auto, auto, auto close) {
            close();
        };
    }

    /**
     * Echo back the request details in SSE format (for testing headers, methods, etc.)
     */
    static MockSSEServer::RequestHandler echo() {
        return [](auto const& req, auto send_response, auto send_sse_event, auto close) {
            http::response<http::string_body> res{http::status::ok, 11};
            res.set(http::field::content_type, "text/event-stream");
            res.chunked(true);

            send_response(res);

            std::string info;
            info += "method: " + std::string(req.method_string()) + "\n";
            info += "target: " + std::string(req.target()) + "\n";

            for (auto const& field : req) {
                info += std::string(field.name_string()) + ": " +
                       std::string(field.value()) + "\n";
            }

            if (!req.body().empty()) {
                info += "body: " + req.body() + "\n";
            }

            if (!send_sse_event(SSEFormatter::event(info))) {
                return;  // Connection closed or error
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            close();
        };
    }
};

} // namespace launchdarkly::sse::test
