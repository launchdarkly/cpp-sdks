#pragma once

#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/optional.hpp>
#include <deque>
#include <launchdarkly/sse/detail/sse_stream.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <functional>
#include <deque>
#include <vector>

namespace launchdarkly::sse {

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;  // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

class client;

class builder {
public:
    builder(net::any_io_executor ioc, std::string url);
    builder& header(const std::string& name, const std::string& value);
    builder& method(http::verb verb);
    std::shared_ptr<client> build();

   private:
    std::string m_url;
    net::any_io_executor m_executor;
    ssl::context m_ssl_ctx;
    http::request<http::empty_body> m_request;
};

class event_data {
    std::string m_type;
    std::string m_data;
    boost::optional<std::string> m_id;

   public:
    explicit event_data(boost::optional<std::string> id);
    void set_type(std::string);
    std::string const& get_type();
    std::string const& get_data();
    void append_data(std::string const&);
};

using sse_event = event_data;
using sse_comment = std::string;

using event = std::variant<sse_event, sse_comment>;

class client : public std::enable_shared_from_this<client> {
    using parser =
        http::response_parser<http::basic_dynamic_body<beast::flat_buffer>>;
    tcp::resolver m_resolver;
    beast::ssl_stream<beast::tcp_stream> m_stream;
    beast::flat_buffer m_buffer;
    http::request<http::empty_body> m_request;
    http::response<http::string_body> m_response;
    parser m_parser;
    std::string m_host;
    std::string m_port;
    boost::optional<std::string> m_buffered_line;
    std::deque<std::string> m_complete_lines;
    std::vector<event> m_events;
    bool m_begin_CR;
    boost::optional<event_data> m_event_data;
    std::function<void(event_data)> m_cb;
    void complete_line();
    size_t append_up_to(std::string_view body, const std::string& search);
    std::size_t parse_stream(std::uint64_t remain, std::string_view body, beast::error_code& ec);
    void parse_events();
public:
    explicit client(net::any_io_executor ex, ssl::context &ctx, http::request<http::empty_body> req, std::string host, std::string port);
    void read();
    template<typename Callback>
    void on_event(Callback event_cb) {
        m_cb = event_cb;
    }
};

}  // namespace launchdarkly::sse
