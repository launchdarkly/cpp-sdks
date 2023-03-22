#pragma once

#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/optional.hpp>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
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
    builder& header(std::string const& name, std::string const& value);
    builder& method(http::verb verb);
    builder& tls(ssl::context_base::method);
    std::shared_ptr<client> build();

   private:
    std::string m_url;
    net::any_io_executor m_executor;
    ssl::context m_ssl_ctx;
    http::request<http::empty_body> m_request;
    std::optional<unsigned int> tls_version_;
};

class event_data {
    std::string m_type;
    std::string m_data;
    std::optional<std::string> m_id;

   public:
    explicit event_data();
    void set_type(std::string);
    void set_id(std::optional<std::string>);
    void append_data(std::string const&);
    std::string const& get_type();
    std::string const& get_data();
    std::optional<std::string> const& get_id();
};

using sse_event = event_data;
using sse_comment = std::string;

using event = std::variant<sse_event, sse_comment>;

class client : public std::enable_shared_from_this<client> {
   protected:
    using parser = http::response_parser<http::string_body>;
    tcp::resolver m_resolver;
    beast::flat_buffer m_buffer;
    http::request<http::empty_body> m_request;
    http::response<http::string_body> m_response;
    parser parser_;
    std::string host_;
    std::string port_;
    std::optional<std::string> buffered_line_;
    std::deque<std::string> complete_lines_;
    std::vector<event> m_events;
    std::optional<std::string> last_event_id_;
    bool begin_CR_;
    std::optional<event_data> m_event_data;
    std::function<void(event_data)> m_cb;
    void complete_line();
    size_t append_up_to(boost::string_view body, std::string const& search);
    std::size_t parse_stream(std::uint64_t remain,
                             boost::string_view body,
                             beast::error_code& ec);
    void parse_events();

    std::optional<std::function<
        size_t(uint64_t, boost::string_view, boost::system::error_code&)>>
        on_chunk_body_trampoline_;

   public:
    client(boost::asio::any_io_executor ex,
           http::request<http::empty_body> req,
           std::string host,
           std::string port);

    template <typename Callback>
    void on_event(Callback event_cb) {
        m_cb = event_cb;
    }

    virtual void read() = 0;
};

}  // namespace launchdarkly::sse
