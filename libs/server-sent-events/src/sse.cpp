#include <launchdarkly/sse/sse.hpp>
#include <boost/url/parse.hpp>
#include <iostream>

namespace launchdarkly {
namespace sse {

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

Builder::Builder(std::string url):
    m_url{url},
    m_ssl_ctx{ssl::context::tlsv12_client} {
}

std::shared_ptr<Client> Builder::build() {
    boost::system::result<boost::urls::url_view> uri_components = boost::urls::parse_uri(m_url);
    if (!uri_components) {
        return nullptr;
    }
    auto client = std::make_shared<Client>(net::make_strand(m_executor), m_ssl_ctx);
    client->run(uri_components->host(), uri_components->port(), uri_components->path());
    return client;
}


void fail(beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

Client::Client(net::any_io_executor ex, ssl::context &ctx):
    m_resolver{ex},
    m_stream{ex, ctx} {}


void Client::run(const std::string& host, const std::string& port, const std::string& target) {
    // Set SNI Hostname (many hosts need this to handshake successfully)
    if(!SSL_set_tlsext_host_name(m_stream.native_handle(), host.c_str()))
    {
        beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
        std::cerr << ec.message() << "\n";
        return;
    }

    // Set up an HTTP GET request message
    m_request.version(11);
    m_request.method(http::verb::get);
    m_request.target(target);
    m_request.set(http::field::host, host);
    m_request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // Look up the domain name
    m_resolver.async_resolve(
            host,
            port,
            beast::bind_front_handler(
                    &Client::on_resolve,
                    shared_from_this()));
}

void Client::on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
    if (ec) {
        return fail(ec, "resolve");
    }

    // Set a timeout on the operation
    beast::get_lowest_layer(m_stream).expires_after(std::chrono::seconds(10));

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(m_stream).async_connect(
            results,
            beast::bind_front_handler(
                    &Client::on_connect,
                    shared_from_this()));
}

void Client::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type endpoint) {
    if (ec) {
        return fail(ec, "connect");
    }

    m_stream.async_handshake(
        ssl::stream_base::client,
        beast::bind_front_handler(
            &Client::on_handshake,
            shared_from_this()
        )
    );
}
void Client::on_handshake(beast::error_code ec) {
    if (ec) {
        return fail(ec, "handshake");
    }

    beast::get_lowest_layer(m_stream).expires_after(std::chrono::seconds(10));

    http::async_write(m_stream, m_request,
        beast::bind_front_handler(
              &Client::on_write,
              shared_from_this()
        )
    );

}
void Client::on_write(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if (ec) {
        return fail(ec, "write");
    }
    http::async_read(m_stream, m_buffer, m_response,
         beast::bind_front_handler(
             &Client::on_read,
             shared_from_this()
         )
    );
}
void Client::on_read(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if (ec) {
        return fail(ec, "read");
    }
    std::cout << m_response << std::endl;
    std::cout << "queueing async read" << std::endl;
    http::async_read(m_stream, m_buffer, m_response,
        beast::bind_front_handler(
            &Client::on_read,
            shared_from_this()
        )
    );
}
void Client::on_shutdown(beast::error_code ec) {
    if(ec == net::error::eof) {
        // Rationale:
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        ec = {};
    }
    if (ec) {
        return fail(ec, "shutdown");
    }
}

} // namespace sse
} // namespace launchdarkly
