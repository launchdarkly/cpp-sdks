#include <launchdarkly/sse/client.hpp>
#include <launchdarkly/sse/detail/parser.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

#include <boost/url/parse.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <tuple>

namespace launchdarkly::sse {

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;  // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

char const* kUserAgent = "CPPClient/0.0.0";

template <class Derived>
class Session {
   private:
    Derived& derived() { return static_cast<Derived&>(*this); }
    http::request<http::empty_body> req_;
    std::chrono::seconds connect_timeout_;
    std::chrono::seconds write_timeout_;

   protected:
    beast::flat_buffer buffer_;
    std::string host_;
    std::string port_;
    tcp::resolver resolver_;

    using cb = std::function<void(launchdarkly::sse::Event)>;
    using body = launchdarkly::sse::detail::EventBody<cb>;
    http::response_parser<body> parser_;

   public:
    Session(net::any_io_executor exec,
            std::string host,
            std::string port,
            http::request<http::empty_body> r,
            std::chrono::seconds connect_timeout,
            Builder::EventReceiver receiver)
        : req_(std::move(r)),
          resolver_(std::move(exec)),
          connect_timeout_(connect_timeout),
          write_timeout_(std::chrono::seconds(10)),
          host_(std::move(host)),
          port_(std::move(port)),
          parser_() {
        parser_.get().body().on_event(std::move(receiver));
    }

    void do_write() {
        http::async_write(
            derived().stream(), req_,
            beast::bind_front_handler(&Session::on_write,
                                      derived().shared_from_this()));
    }

    void do_resolve() {
        resolver_.async_resolve(
            host_, port_,
            beast::bind_front_handler(&Session::on_resolve,
                                      derived().shared_from_this()));
    }

    void fail(beast::error_code ec, char const* what) {
        // log(std::string(what) + ":" + ec.message());
    }

    void on_connect(beast::error_code ec,
                    tcp::resolver::results_type::endpoint_type eps) {
        if (ec) {
            return fail(ec, "connect");
        }
        derived().do_handshake();
    }

    void on_handshake(beast::error_code ec) {
        if (ec)
            return fail(ec, "handshake");

        do_write();
    }

    void on_write(beast::error_code ec, std::size_t) {
        if (ec)
            return fail(ec, "write");

        beast::get_lowest_layer(derived().stream())
            .expires_after(write_timeout_);

        http::async_read_some(
            derived().stream(), buffer_, parser_,
            beast::bind_front_handler(&Session::on_read,
                                      derived().shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec) {
            return fail(ec, "read");
        }

        beast::get_lowest_layer(derived().stream()).expires_never();

        http::async_read_some(
            derived().stream(), buffer_, parser_,
            beast::bind_front_handler(&Session::on_read,
                                      derived().shared_from_this()));
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec)
            return fail(ec, "resolve");

        beast::get_lowest_layer(derived().stream())
            .expires_after(connect_timeout_);

        beast::get_lowest_layer(derived().stream())
            .async_connect(results, beast::bind_front_handler(
                                        &Session::on_connect,
                                        derived().shared_from_this()));
    }

    void do_close() { beast::get_lowest_layer(derived().stream()).cancel(); }
};

class SecureClient : public Client,
                     public Session<SecureClient>,
                     public std::enable_shared_from_this<SecureClient> {
   public:
    SecureClient(net::any_io_executor ex,
                 ssl::context ctx,
                 http::request<http::empty_body> req,
                 std::string host,
                 std::string port,
                 Builder::EventReceiver receiver)
        : Session<SecureClient>(ex,
                                host,
                                port,
                                req,
                                std::chrono::seconds(5),
                                std::move(receiver)),
          ssl_ctx_(std::move(ctx)),
          stream_{ex, ssl_ctx_} {}

    virtual void run() override {
        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(stream_.native_handle(), host_.c_str())) {
            beast::error_code ec{static_cast<int>(::ERR_get_error()),
                                 net::error::get_ssl_category()};
            //     log("failed to set TLS host name extension: " +
            //     ec.message());
            return;
        }

        do_resolve();
    }

    virtual void close() override { do_close(); }

    void do_handshake() {
        stream_.async_handshake(
            ssl::stream_base::client,
            beast::bind_front_handler(&SecureClient::on_handshake, shared()));
    }

    beast::ssl_stream<beast::tcp_stream>& stream() { return stream_; }

   private:
    ssl::context ssl_ctx_;
    beast::ssl_stream<beast::tcp_stream> stream_;

    std::shared_ptr<SecureClient> shared() {
        return std::static_pointer_cast<SecureClient>(shared_from_this());
    }
};

class PlaintextClient : public Client,
                        public Session<PlaintextClient>,
                        public std::enable_shared_from_this<PlaintextClient> {
   public:
    PlaintextClient(net::any_io_executor ex,
                    http::request<http::empty_body> req,
                    std::string host,
                    std::string port,
                    Builder::EventReceiver receiver)
        : Session<PlaintextClient>(ex,
                                   host,
                                   port,
                                   req,
                                   std::chrono::seconds(5),
                                   std::move(receiver)),
          stream_{ex} {}

    virtual void run() override { do_resolve(); }

    void do_handshake() {
        // No handshake for plaintext; immediately send the request instead.
        do_write();
    }

    virtual void close() override { do_close(); }

    beast::tcp_stream& stream() { return stream_; }

   private:
    beast::tcp_stream stream_;
};

Builder::Builder(net::any_io_executor ctx, std::string url)
    : url_{std::move(url)}, executor_{std::move(ctx)} {
    receiver_ = [](launchdarkly::sse::Event) {};

    request_.version(11);
    request_.set(http::field::user_agent, kUserAgent);
    request_.method(http::verb::get);
    request_.set("Accept", "text/event-stream");
    request_.set("Cache-Control", "no-cache");
}

Builder& Builder::header(std::string const& name, std::string const& value) {
    request_.set(name, value);
    return *this;
}

Builder& Builder::method(http::verb verb) {
    request_.method(verb);
    return *this;
}

Builder& Builder::receiver(EventReceiver receiver) {
    receiver_ = std::move(receiver);
    return *this;
}

Builder& Builder::logging(std::function<void(std::string)> cb) {
    logging_cb_ = std::move(cb);
    return *this;
}

std::shared_ptr<Client> Builder::build() {
    auto uri_components = boost::urls::parse_uri(url_);
    if (!uri_components) {
        return nullptr;
    }

    std::string host = uri_components->host();

    request_.set(http::field::host, host);
    request_.target(uri_components->path());

    if (uri_components->scheme_id() == boost::urls::scheme::https) {
        std::string port =
            uri_components->has_port() ? uri_components->port() : "443";

        ssl::context ssl_ctx{ssl::context::tlsv12_client};
        // TODO: This needs to be verify_peer in production,
        // and we need to provide a certificate store!
        ssl_ctx.set_verify_mode(ssl::verify_none);

        return std::make_shared<SecureClient>(net::make_strand(executor_),
                                              std::move(ssl_ctx), request_,
                                              host, port, receiver_);
    } else {
        std::string port =
            uri_components->has_port() ? uri_components->port() : "80";

        return std::make_shared<PlaintextClient>(
            net::make_strand(executor_), request_, host, port, receiver_);
    }
}

}  // namespace launchdarkly::sse
