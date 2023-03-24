#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/ssl.hpp>

#include <boost/beast/http.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/core/flat_buffer.hpp>

#include <string>
#include <functional>
#include <memory>
#include <optional>
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
    builder& header(std::string const& name, std::string const& value);
    builder& method(http::verb verb);
    builder& tls(ssl::context_base::method);
    builder& logging(std::function<void(std::string)> callback);
    std::shared_ptr<client> build();

   private:
    std::string url_;
    net::any_io_executor executor_;
    ssl::context ssl_context_;
    http::request<http::empty_body> request_;
    std::function<void(std::string)> logging_cb_;
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
    void trim_trailing_newline();
    std::string const& get_type();
    std::string const& get_data();
    std::optional<std::string> const& get_id();
};

using sse_event = event_data;
using sse_comment = std::string;

using event = std::variant<sse_event, sse_comment>;

class client : public std::enable_shared_from_this<client> {
   public:
    using logger = std::function<void(std::string)>;

    client(boost::asio::any_io_executor ex,
           http::request<http::empty_body> req,
           std::string host,
           std::string port,
           logger logger, std::string log_tag);
    ~client();

    template <typename Callback>
    void on_event(Callback event_cb) {
        event_callback_ = event_cb;
    }

    virtual void run() = 0;
    virtual void close() = 0;

   protected:
    using parser = http::response_parser<http::string_body>;
    tcp::resolver resolver_;
    beast::flat_buffer buffer_;
    http::request<http::empty_body> request_;
    http::response<http::string_body> response_;
    parser parser_;
    std::string host_;
    std::string port_;
    std::optional<std::string> buffered_line_;
    std::deque<std::string> complete_lines_;
    std::vector<event> events_;
    std::optional<std::string> last_event_id_;
    bool begin_CR_;
    std::optional<event_data> event_buffer_;
    std::function<void(event_data)> event_callback_;
    logger logging_cb_;
    std::string log_tag_;

    void complete_line();
    void parse_events();
    size_t append_up_to(boost::string_view body, std::string const& search);
    std::size_t parse_stream(std::uint64_t remain,
                             boost::string_view body,
                             beast::error_code& ec);

    std::optional<std::function<
        size_t(uint64_t, boost::string_view, boost::system::error_code&)>>
        on_chunk_body_trampoline_;

    void log(std::string);
    void fail(beast::error_code ec, char const* what);


};

}  // namespace launchdarkly::sse
