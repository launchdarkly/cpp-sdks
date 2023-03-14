#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <string>
#include <memory>

namespace launchdarkly {
namespace sse {

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class Client;

class Builder {
    std::string m_url;
    net::any_io_executor m_executor;
    ssl::context m_ssl_ctx;
public:
    Builder(std::string url);
    std::shared_ptr<Client> build();
};

class Client : public std::enable_shared_from_this<Client> {
    tcp::resolver m_resolver;
    beast::ssl_stream<beast::tcp_stream> m_stream;
    beast::flat_buffer m_buffer;
    http::request<http::empty_body> m_request;
    http::response<http::string_body> m_response;
public:
    explicit Client(net::any_io_executor ex, ssl::context &ctx);
    void run(const std::string& host, const std::string& port, const std::string& target);
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type endpoint);
    void on_handshake(beast::error_code ec);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_shutdown(beast::error_code ec);
};




} // namespace sse
} // namespace launchdarkly
