#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <memory>
#include <string>
#include <sstream>
#include <iostream>
#include <functional>
#include <thread>
#include <chrono>
#include <atomic>

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
        std::function<void(std::string const&)> send_sse_event,
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
        std::cout << "[MockServer] start: initializing server" << std::endl;
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

        std::cout << "[MockServer] Server bound to port " << port_ << std::endl;

        // Start accepting connections in a background thread
        server_thread_ = std::thread([this]() {
            std::cout << "[MockServer] Background thread started" << std::endl;
            do_accept();
            std::cout << "[MockServer] About to run io_context" << std::endl;
            ioc_.run();
            std::cout << "[MockServer] io_context.run() exited" << std::endl;
        });

        std::cout << "[MockServer] Server started on port " << port_ << std::endl;
        return port_;
    }

    void stop() {
        if (!running_) {
            return;
        }

        running_ = false;

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
            std::cout << "[MockServer] do_accept: not running, returning" << std::endl;
            return;
        }

        std::cout << "[MockServer] do_accept: waiting for connection on port " << port_ << std::endl;
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (ec) {
                    std::cout << "[MockServer] async_accept error: " << ec.message() << std::endl;
                } else {
                    std::cout << "[MockServer] Connection accepted from "
                              << socket.remote_endpoint().address().to_string()
                              << ":" << socket.remote_endpoint().port() << std::endl;
                    handle_connection(std::move(socket));
                }

                if (running_) {
                    do_accept();
                }
            });
    }

    void handle_connection(tcp::socket socket) {
        std::cout << "[MockServer] handle_connection: creating Connection object" << std::endl;
        auto conn = std::make_shared<Connection>(
            std::move(socket), handler_);
        conn->start();
        std::cout << "[MockServer] handle_connection: Connection started" << std::endl;
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
            std::cout << "[Connection] start: beginning read" << std::endl;
            do_read();
        }

        void do_read() {
            std::cout << "[Connection] do_read: async_read started" << std::endl;
            auto self = shared_from_this();
            http::async_read(
                socket_,
                buffer_,
                req_,
                [self](boost::system::error_code ec, std::size_t bytes) {
                    if (ec) {
                        std::cout << "[Connection] async_read error: " << ec.message() << std::endl;
                        return;
                    }
                    std::cout << "[Connection] async_read success: " << bytes << " bytes read" << std::endl;
                    std::cout << "[Connection] Request: " << self->req_.method_string() << " "
                              << self->req_.target() << std::endl;
                    self->handle_request();
                });
        }

        void handle_request() {
            std::cout << "[Connection] handle_request: setting up callbacks" << std::endl;
            auto self = shared_from_this();

            auto send_response = [self](http::response<http::string_body> res) {
                std::cout << "[Connection] send_response called: status=" << res.result_int()
                          << ", chunked=" << res.chunked() << std::endl;
                if (self->closed_) {
                    std::cout << "[Connection] send_response: already closed, skipping" << std::endl;
                    return;
                }

                boost::system::error_code ec;

                // For error responses and redirects (with body or no SSE), write complete response
                if (res.result() != http::status::ok || !res.chunked()) {
                    std::cout << "[Connection] Sending complete response" << std::endl;
                    http::write(self->socket_, res, ec);
                    if (ec) {
                        std::cout << "[Connection] Error writing response: " << ec.message() << std::endl;
                    } else {
                        std::cout << "[Connection] Complete response sent successfully" << std::endl;
                    }
                    return;
                }

                // For SSE (chunked OK responses), manually send headers to keep connection open
                std::cout << "[Connection] Sending SSE response headers" << std::endl;
                std::ostringstream oss;
                oss << "HTTP/1.1 " << res.result_int() << " " << res.reason() << "\r\n";
                for (auto const& field : res) {
                    oss << field.name_string() << ": " << field.value() << "\r\n";
                }
                oss << "\r\n";  // End of headers

                std::string header_str = oss.str();
                std::cout << "[Connection] Headers to send:\n" << header_str << std::endl;
                net::write(self->socket_, net::buffer(header_str), ec);
                if (ec) {
                    std::cout << "[Connection] Error writing headers: " << ec.message() << std::endl;
                } else {
                    std::cout << "[Connection] Headers sent successfully, " << header_str.size() << " bytes" << std::endl;
                }
            };

            auto send_sse_event = [self](std::string const& data) {
                std::cout << "[Connection] send_sse_event called: " << data.size() << " bytes" << std::endl;
                if (self->closed_) {
                    std::cout << "[Connection] send_sse_event: already closed, skipping" << std::endl;
                    return;
                }

                boost::system::error_code ec;

                // Send as chunked encoding: size in hex + CRLF + data + CRLF
                std::ostringstream chunk;
                chunk << std::hex << data.size() << "\r\n" << data << "\r\n";
                std::string chunk_str = chunk.str();

                std::cout << "[Connection] Sending chunk: size=" << data.size()
                          << ", total chunk size=" << chunk_str.size() << " bytes" << std::endl;

                net::write(self->socket_, net::buffer(chunk_str), ec);
                if (ec) {
                    std::cout << "[Connection] Error writing SSE data: " << ec.message() << std::endl;
                } else {
                    std::cout << "[Connection] SSE chunk sent successfully" << std::endl;
                }
            };

            auto close_connection = [self]() {
                std::cout << "[Connection] close_connection called" << std::endl;
                if (self->closed_) {
                    std::cout << "[Connection] Already closed" << std::endl;
                    return;
                }
                self->closed_ = true;

                // Send final chunk terminator for chunked encoding
                boost::system::error_code ec;
                std::string final_chunk = "0\r\n\r\n";
                std::cout << "[Connection] Sending final chunk terminator" << std::endl;
                net::write(self->socket_, net::buffer(final_chunk), ec);
                if (ec) {
                    std::cout << "[Connection] Error writing final chunk: " << ec.message() << std::endl;
                } else {
                    std::cout << "[Connection] Final chunk sent" << std::endl;
                }

                self->socket_.shutdown(tcp::socket::shutdown_both, ec);
                if (ec) {
                    std::cout << "[Connection] Error during shutdown: " << ec.message() << std::endl;
                }
                self->socket_.close(ec);
                if (ec) {
                    std::cout << "[Connection] Error during close: " << ec.message() << std::endl;
                } else {
                    std::cout << "[Connection] Connection closed successfully" << std::endl;
                }
            };

            std::cout << "[Connection] Calling handler" << std::endl;
            handler_(req_, send_response, send_sse_event, close_connection);
            std::cout << "[Connection] Handler returned" << std::endl;
        }
    };

    net::io_context ioc_;
    tcp::acceptor acceptor_;
    std::thread server_thread_;
    RequestHandler handler_;
    std::atomic<bool> running_;
    std::atomic<uint16_t> port_;
    bool use_ssl_;
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

            // Send response headers
            send_response(res);

            // Send SSE event
            send_sse_event(SSEFormatter::event(data));

            // Close connection after a brief delay
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
                send_sse_event(SSEFormatter::event(data));
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

            send_sse_event(SSEFormatter::event(info));

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            close();
        };
    }
};

} // namespace launchdarkly::sse::test
