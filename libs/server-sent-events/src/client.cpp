#include <launchdarkly/sse/client.hpp>

#include <boost/asio/placeholders.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/optional/optional.hpp>
#include <boost/url/parse.hpp>
#include <iostream>
#include <memory>
#include <tuple>

namespace launchdarkly::sse {

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;  // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

client::client(net::any_io_executor ex,
               http::request<http::empty_body> req,
               std::string host,
               std::string port,
               std::function<void(std::string)> logging_cb,
               std::string log_tag)
    : resolver_{ex},
      buffer_{},
      parser_{},
      host_{std::move(host)},
      port_{std::move(port)},
      request_{std::move(req)},
      response_{},
      log_tag_{std::move(log_tag)},
      logging_cb_{std::move(logging_cb)} {
    parser_.body_limit(boost::none);

    on_event([this](Event e) {
        log("got event: (" + e.type() + ", " + e.data() + ")");
    });

    log("create");
}

client::~client() {
    log("destroy");
}

void client::log(std::string what) {
    if (logging_cb_) {
        logging_cb_(log_tag_ + ": " + std::move(what));
    }
}

// Report a failure
void client::fail(beast::error_code ec, char const* what) {
    log(std::string(what) + ":" + ec.message());
}

class ssl_client : public client {
   public:
    ssl_client(net::any_io_executor ex,
               ssl::context& ctx,
               http::request<http::empty_body> req,
               std::string host,
               std::string port,
               logger logging_cb)
        : client(ex,
                 std::move(req),
                 std::move(host),
                 std::move(port),
                 std::move(logging_cb),
                 "sse-tls"),
          stream_{ex, ctx} {}

    void run() override {
        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(stream_.native_handle(), host_.c_str())) {
            beast::error_code ec{static_cast<int>(::ERR_get_error()),
                                 net::error::get_ssl_category()};
            log("failed to set TLS host name extension: " + ec.message());
            return;
        }

        beast::get_lowest_layer(stream_).expires_after(
            std::chrono::seconds(15));

        resolver_.async_resolve(
            host_, port_,
            beast::bind_front_handler(&ssl_client::on_resolve, shared()));
    }

    void close() override {
        net::post(stream_.get_executor(),
                  beast::bind_front_handler(&ssl_client::on_stop, shared()));
    }

   private:
    beast::ssl_stream<beast::tcp_stream> stream_;

    std::shared_ptr<ssl_client> shared() {
        return std::static_pointer_cast<ssl_client>(shared_from_this());
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec)
            return fail(ec, "resolve");
        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(stream_).async_connect(
            results,
            beast::bind_front_handler(&ssl_client::on_connect, shared()));
    }

    void on_connect(beast::error_code ec,
                    tcp::resolver::results_type::endpoint_type) {
        if (ec)
            return fail(ec, "connect");

        stream_.async_handshake(
            ssl::stream_base::client,
            beast::bind_front_handler(&ssl_client::on_handshake, shared()));
    }

    void on_handshake(beast::error_code ec) {
        if (ec)
            return fail(ec, "handshake");

        // Send the HTTP request to the remote host
        http::async_write(
            stream_, request_,
            beast::bind_front_handler(&ssl_client::on_write, shared()));
    }

    void on_write(beast::error_code ec, std::size_t) {
        if (ec)
            return fail(ec, "write");

        beast::get_lowest_layer(stream_).expires_after(
            std::chrono::seconds(10));

        http::async_read_some(
            stream_, buffer_, parser_,
            beast::bind_front_handler(&ssl_client::on_read, shared()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec) {
            return fail(ec, "read");
        }

        beast::get_lowest_layer(stream_).expires_never();

        http::async_read_some(
            stream_, buffer_, parser_,
            beast::bind_front_handler(&ssl_client::on_read, shared()));
    }

    void on_stop() { beast::get_lowest_layer(stream_).cancel(); }
};

class plaintext_client : public client {
   public:
    plaintext_client(net::any_io_executor ex,
                     ssl::context& ctx,
                     http::request<http::empty_body> req,
                     std::string host,
                     std::string port,
                     logger logger)
        : client(ex,
                 std::move(req),
                 std::move(host),
                 std::move(port),
                 std::move(logger),
                 "sse-plaintext"),
          stream_{ex},
          ctx_{ctx} {}

    void run() override {
        beast::get_lowest_layer(stream_).expires_after(
            std::chrono::seconds(15));

        resolver_.async_resolve(
            host_, port_,
            beast::bind_front_handler(&plaintext_client::on_resolve, shared()));
    }

    void close() override {
        net::post(
            stream_.get_executor(),
            beast::bind_front_handler(&plaintext_client::on_stop, shared()));
    }

   private:
    beast::tcp_stream stream_;
    ssl::context& ctx_;

    std::shared_ptr<plaintext_client> shared() {
        return std::static_pointer_cast<plaintext_client>(shared_from_this());
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec)
            return fail(ec, "resolve");
        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(stream_).async_connect(
            results,
            beast::bind_front_handler(&plaintext_client::on_connect, shared()));
    }

    void on_connect(beast::error_code ec,
                    tcp::resolver::results_type::endpoint_type) {
        if (ec)
            return fail(ec, "connect");

        http::async_write(
            stream_, request_,
            beast::bind_front_handler(&plaintext_client::on_write, shared()));
    }

    void on_write(beast::error_code ec, std::size_t) {
        if (ec)
            return fail(ec, "write");

        beast::get_lowest_layer(stream_).expires_after(
            std::chrono::seconds(10));

        http::async_read_some(
            stream_, buffer_, parser_,
            beast::bind_front_handler(&plaintext_client::on_read, shared()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec && ec != beast::errc::operation_canceled) {
            return fail(ec, "read");
        }

        http::async_read_some(
            stream_, buffer_, parser_,
            beast::bind_front_handler(&plaintext_client::on_read, shared()));
    }

    void on_stop() { stream_.cancel(); }
};

builder::builder(net::any_io_executor ctx, std::string url)
    : url_{std::move(url)},
      ssl_context_{ssl::context::tlsv12_client},
      executor_{std::move(ctx)} {
    // TODO: This needs to be verify_peer in production!!
    ssl_context_.set_verify_mode(ssl::verify_none);

    request_.version(11);
    request_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    request_.method(http::verb::get);
    request_.set("Accept", "text/event-stream");
    request_.set("Cache-Control", "no-cache");
}

builder& builder::header(std::string const& name, std::string const& value) {
    request_.set(name, value);
    return *this;
}

builder& builder::method(http::verb verb) {
    request_.method(verb);
    return *this;
}

builder& builder::tls(ssl::context_base::method ctx) {
    ssl_context_ = ssl::context{ctx};
    return *this;
}

builder& builder::logging(std::function<void(std::string)> cb) {
    logging_cb_ = std::move(cb);
    return *this;
}

std::shared_ptr<client> builder::build() {
    boost::system::result<boost::urls::url_view> uri_components =
        boost::urls::parse_uri(url_);
    if (!uri_components) {
        return nullptr;
    }

    request_.set(http::field::host, uri_components->host());
    request_.target(uri_components->path());

    if (uri_components->scheme_id() == boost::urls::scheme::https) {
        std::string port =
            uri_components->has_port() ? uri_components->port() : "443";

        return std::make_shared<ssl_client>(
            net::make_strand(executor_), ssl_context_, request_,
            uri_components->host(), port, logging_cb_);
    } else {
        std::string port =
            uri_components->has_port() ? uri_components->port() : "80";

        return std::make_shared<plaintext_client>(
            net::make_strand(executor_), ssl_context_, request_,
            uri_components->host(), port, logging_cb_);
    }
}

}  // namespace launchdarkly::sse
