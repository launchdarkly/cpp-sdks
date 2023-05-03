#include <launchdarkly/sse/client.hpp>
#include <launchdarkly/sse/detail/parser.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/host_name_verification.hpp>
#include <boost/asio/strand.hpp>

#include <boost/certify/extensions.hpp>
#include <boost/certify/https_verification.hpp>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/version.hpp>

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

auto const kDefaultUserAgent = BOOST_BEAST_VERSION_STRING;

// The allowed amount of time to connect the socket and perform
// any TLS handshake, if necessary.
const std::chrono::milliseconds kDefaultConnectTimeout =
    std::chrono::seconds(15);
// Once connected, the amount of time to send a request and receive the first
// batch of bytes back.
const std::chrono::milliseconds kDefaultResponseTimeout =
    std::chrono::seconds(15);

template <class Derived>
class
    Session {  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
   private:
    Derived& derived() { return static_cast<Derived&>(*this); }
    http::request<http::string_body> req_;
    std::chrono::milliseconds connect_timeout_;
    std::chrono::milliseconds response_timeout_;
    std::optional<std::chrono::milliseconds> read_timeout_;

   protected:
    beast::flat_buffer buffer_;
    std::string host_;
    std::string port_;
    tcp::resolver resolver_;
    Builder::LogCallback logger_;

    using cb = std::function<void(launchdarkly::sse::Event)>;
    using body = launchdarkly::sse::detail::EventBody<cb>;
    http::response_parser<body> parser_;

   public:
    Session(net::any_io_executor const& exec,
            std::string host,
            std::string port,
            http::request<http::string_body> req,
            std::chrono::milliseconds connect_timeout,
            std::chrono::milliseconds response_timeout,
            std::optional<std::chrono::milliseconds> read_timeout,
            Builder::EventReceiver receiver,
            Builder::LogCallback logger)
        : req_(std::move(req)),
          resolver_(exec),
          connect_timeout_(connect_timeout),
          response_timeout_(response_timeout),
          read_timeout_(std::move(read_timeout)),
          host_(std::move(host)),
          port_(std::move(port)),
          logger_(std::move(logger)),
          parser_() {
        parser_.get().body().on_event(std::move(receiver));
    }

    void fail(beast::error_code ec, char const* what) {
        logger_(std::string(what) + ": " + ec.message());
    }

    void do_resolve() {
        logger_("resolving " + host_ + ":" + port_);
        resolver_.async_resolve(
            host_, port_,
            beast::bind_front_handler(&Session::on_resolve,
                                      derived().shared_from_this()));
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec)
            return fail(ec, "resolve");

        logger_("connecting (" + std::to_string(connect_timeout_.count()) +
                " sec timeout)");

        beast::get_lowest_layer(derived().stream())
            .expires_after(connect_timeout_);

        beast::get_lowest_layer(derived().stream())
            .async_connect(results, beast::bind_front_handler(
                                        &Session::on_connect,
                                        derived().shared_from_this()));
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

    void do_write() {
        logger_("making request (" + std::to_string(response_timeout_.count()) +
                " sec timeout)");

        beast::get_lowest_layer(derived().stream())
            .expires_after(response_timeout_);

        http::async_write(
            derived().stream(), req_,
            beast::bind_front_handler(&Session::on_write,
                                      derived().shared_from_this()));
    }

    void on_write(beast::error_code ec, std::size_t) {
        if (ec)
            return fail(ec, "write");

        logger_("reading response");

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

        if (read_timeout_) {
            beast::get_lowest_layer(derived().stream())
                .expires_after(*read_timeout_);
        } else {
            beast::get_lowest_layer(derived().stream()).expires_never();
        };

        http::async_read_some(
            derived().stream(), buffer_, parser_,
            beast::bind_front_handler(&Session::on_read,
                                      derived().shared_from_this()));
    }

    void do_close() {
        logger_("closing");
        beast::get_lowest_layer(derived().stream()).cancel();
    }
};

class EncryptedClient : public Client,
                        public Session<EncryptedClient>,
                        public std::enable_shared_from_this<EncryptedClient> {
   public:
    EncryptedClient(net::any_io_executor ex,
                    ssl::context ctx,
                    http::request<http::string_body> req,
                    std::string host,
                    std::string port,
                    std::optional<std::chrono::milliseconds> read_timeout,
                    Builder::EventReceiver receiver,
                    Builder::LogCallback logger)
        : Session<EncryptedClient>(ex,
                                   std::move(host),
                                   std::move(port),
                                   std::move(req),
                                   kDefaultConnectTimeout,
                                   kDefaultResponseTimeout,
                                   std::move(read_timeout),
                                   std::move(receiver),
                                   std::move(logger)),
          ssl_ctx_(std::move(ctx)),
          stream_{ex, ssl_ctx_} {}

    virtual void run() override {
        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(stream_.native_handle(), host_.c_str())) {
            beast::error_code ec{static_cast<int>(::ERR_get_error()),
                                 net::error::get_ssl_category()};
            logger_("failed to set TLS host name extension: " + ec.message());
            return;
        }

        do_resolve();
    }

    virtual void close() override { do_close(); }

    void do_handshake() {
        stream_.async_handshake(ssl::stream_base::client,
                                beast::bind_front_handler(
                                    &EncryptedClient::on_handshake, shared()));
    }

    beast::ssl_stream<beast::tcp_stream>& stream() { return stream_; }

   private:
    ssl::context ssl_ctx_;
    beast::ssl_stream<beast::tcp_stream> stream_;

    std::shared_ptr<EncryptedClient> shared() {
        return std::static_pointer_cast<EncryptedClient>(shared_from_this());
    }
};

class PlaintextClient : public Client,
                        public Session<PlaintextClient>,
                        public std::enable_shared_from_this<PlaintextClient> {
   public:
    PlaintextClient(net::any_io_executor ex,
                    http::request<http::string_body> req,
                    std::string host,
                    std::string port,
                    std::optional<std::chrono::milliseconds> read_timeout,
                    Builder::EventReceiver receiver,
                    Builder::LogCallback logger)
        : Session<PlaintextClient>(ex,
                                   std::move(host),
                                   std::move(port),
                                   std::move(req),
                                   kDefaultConnectTimeout,
                                   kDefaultResponseTimeout,
                                   read_timeout,
                                   std::move(receiver),
                                   std::move(logger)),
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
    : url_{std::move(url)},
      executor_{std::move(ctx)},
      read_timeout_{std::nullopt} {
    receiver_ = [](launchdarkly::sse::Event const&) {};

    request_.version(11);
    request_.set(http::field::user_agent, kDefaultUserAgent);
    request_.method(http::verb::get);
    request_.set(http::field::accept, "text/event-stream");
    request_.set(http::field::cache_control, "no-cache");
}

Builder& Builder::header(std::string const& name, std::string const& value) {
    request_.set(name, value);
    return *this;
}

Builder& Builder::body(std::string data) {
    request_.body() = std::move(data);
    return *this;
}

Builder& Builder::read_timeout(std::chrono::milliseconds timeout) {
    read_timeout_ = timeout;
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

Builder& Builder::logger(std::function<void(std::string)> callback) {
    logging_cb_ = std::move(callback);
    return *this;
}

std::shared_ptr<Client> Builder::build() {
    auto uri_components = boost::urls::parse_uri(url_);
    if (!uri_components) {
        return nullptr;
    }

    // Don't send a body unless the method is POST or REPORT
    if (!(request_.method() == http::verb::post ||
          request_.method() == http::verb::report)) {
        request_.body() = "";
    } else {
        // If it is, then setup Content-Type, only if one wasn't
        // specified.
        if (auto it = request_.find(http::field::content_type);
            it == request_.end()) {
            request_.set(http::field::content_type, "text/plain");
        }
    }

    request_.prepare_payload();

    std::string host = uri_components->host();

    request_.set(http::field::host, host);
    request_.target(uri_components->path());

    if (uri_components->scheme_id() == boost::urls::scheme::https) {
        std::string port =
            uri_components->has_port() ? uri_components->port() : "443";

        ssl::context ssl_ctx{ssl::context::tlsv12_client};

        ssl_ctx.set_verify_mode(ssl::verify_peer |
                                ssl::verify_fail_if_no_peer_cert);

        ssl_ctx.set_default_verify_paths();
        boost::certify::enable_native_https_server_verification(ssl_ctx);

        return std::make_shared<EncryptedClient>(
            net::make_strand(executor_), std::move(ssl_ctx), request_, host,
            port, read_timeout_, receiver_, logging_cb_);
    } else {
        std::string port =
            uri_components->has_port() ? uri_components->port() : "80";

        return std::make_shared<PlaintextClient>(
            net::make_strand(executor_), request_, host, port, read_timeout_,
            receiver_, logging_cb_);
    }
}

}  // namespace launchdarkly::sse
