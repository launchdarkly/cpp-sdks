#include "http_callback.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/url.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

void PostJsonCallback(boost::asio::any_io_executor executor,
                      std::string const& callback_uri,
                      nlohmann::json const& payload) {
    auto uri_result = boost::urls::parse_uri(callback_uri);
    if (!uri_result) {
        return;
    }

    auto uri = *uri_result;
    std::string host(uri.host());
    std::string port = uri.has_port() ? std::string(uri.port()) : "80";
    std::string target(uri.path());
    if (target.empty()) {
        target = "/";
    }

    beast::error_code ec;
    tcp::resolver resolver(executor);
    auto results = resolver.resolve(host, port, ec);
    if (ec) {
        return;
    }

    beast::tcp_stream stream(executor);
    stream.connect(results, ec);
    if (ec) {
        return;
    }

    http::request<http::string_body> req{http::verb::post, target, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, "cpp-server-sdk-contract-tests");
    req.set(http::field::content_type, "application/json");
    req.body() = payload.dump();
    req.prepare_payload();

    http::write(stream, req, ec);
    if (ec) {
        return;
    }

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res, ec);

    beast::error_code ignore_ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ignore_ec);
}
