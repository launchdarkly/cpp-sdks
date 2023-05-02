#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/host_name_verification.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/use_future.hpp>
#include <foxy/client_session.hpp>
#include <launchdarkly/sse/client.hpp>
#include <launchdarkly/sse/detail/parser.hpp>

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
auto const kDefaultConnectTimeout = std::chrono::seconds(15);

// The allowed amount of time to send the initial request.
auto const kDefaultReqTimeout = std::chrono::seconds(15);

// Once the request has been sent, the amount of time between subsequent reads
// of the stream.
auto const kDefaultReadTimeout = std::chrono::minutes(5);

class FoxyClient : public Client,
                   public std::enable_shared_from_this<FoxyClient> {
   public:
    FoxyClient(boost::asio::any_io_executor executor,
               http::request<http::string_body> req,
               std::string host,
               std::string port,
               std::optional<std::chrono::seconds> read_timeout,
               Builder::EventReceiver receiver,
               Builder::LogCallback logger,
               net::ssl::context ssl_context)
        : ssl_context_(std::move(ssl_context)),
          host_(std::move(host)),
          port_(std::move(port)),
          read_timeout_(read_timeout),
          req_(std::move(req)),
          parser_(),
          session_(executor,
                   foxy::session_opts{.ssl_ctx = *ssl_context_,
                                      .timeout = kDefaultConnectTimeout}),
          logger_(std::move(logger)) {
        parser_.get().body().on_event(std::move(receiver));
    }

    FoxyClient(boost::asio::any_io_executor executor,
               http::request<http::string_body> req,
               std::string host,
               std::string port,
               std::optional<std::chrono::seconds> read_timeout,
               Builder::EventReceiver receiver,
               Builder::LogCallback logger)
        : ssl_context_(std::nullopt),
          buffer_(),
          host_(std::move(host)),
          port_(std::move(port)),
          read_timeout_(read_timeout),
          req_(std::move(req)),
          parser_(),
          session_(executor,
                   foxy::session_opts{.timeout = kDefaultConnectTimeout}),
          logger_(std::move(logger)) {
        parser_.get().body().on_event(std::move(receiver));
    }

    void fail(boost::system::error_code ec, std::string what) {
        logger_(what + ":" + ec.message());
    }

    virtual void run() override {
        session_.async_connect(
            host_, port_,
            beast::bind_front_handler(&FoxyClient::on_connect,
                                      shared_from_this()));
    }

    void on_connect(boost::system::error_code ec) {
        if (ec) {
            return fail(ec, "connect");
        }

        session_.opts.timeout = kDefaultReqTimeout;
        session_.async_write(req_,
                             beast::bind_front_handler(&FoxyClient::on_write,
                                                       shared_from_this()));
    }

    void on_write(boost::system::error_code ec, std::size_t size) {
        boost::ignore_unused(size);
        if (ec) {
            return fail(ec, "write");
        }

        session_.opts.timeout = read_timeout_.value_or(kDefaultReadTimeout);
        session_.async_read_some(parser_,
                                 beast::bind_front_handler(&FoxyClient::on_read,
                                                           shared_from_this()));
    }
    void on_read(boost::system::error_code ec, std::size_t size) {
        boost::ignore_unused(size);
        if (ec) {
            return fail(ec, "read");
        }
        session_.async_read_some(parser_,
                                 beast::bind_front_handler(&FoxyClient::on_read,
                                                           shared_from_this()));
    }

    virtual void close() override {
        session_.async_shutdown(boost::asio::use_future).get();
    }

   private:
    std::optional<net::ssl::context> ssl_context_;
    beast::flat_buffer buffer_;
    std::string host_;
    std::string port_;
    std::optional<std::chrono::seconds> read_timeout_;
    http::request<http::string_body> req_;
    using cb = std::function<void(launchdarkly::sse::Event)>;
    using body = launchdarkly::sse::detail::EventBody<cb>;
    http::response_parser<body> parser_;
    foxy::client_session session_;
    Builder::LogCallback logger_;
};

Builder::Builder(net::any_io_executor ctx, std::string url)
    : url_{std::move(url)},
      executor_{std::move(ctx)},
      read_timeout_{std::nullopt},
      logging_cb_([](auto msg) {}) {
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

Builder& Builder::read_timeout(std::chrono::seconds timeout) {
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

    auto request = request_;

    // Don't send a body unless the method is POST or REPORT
    if (!(request.method() == http::verb::post ||
          request.method() == http::verb::report)) {
        request.body() = "";
    } else {
        // If it is, then setup Content-Type, only if one wasn't
        // specified.
        if (auto it = request.find(http::field::content_type);
            it == request.end()) {
            request.set(http::field::content_type, "text/plain");
        }
    }

    request.prepare_payload();

    std::string host = uri_components->host();

    request.set(http::field::host, host);
    request.target(uri_components->path());

    if (uri_components->scheme_id() == boost::urls::scheme::https) {
        std::string port =
            uri_components->has_port() ? uri_components->port() : "443";

        auto ssl_ctx = foxy::make_ssl_ctx(ssl::context::tlsv12_client);

        ssl_ctx.set_default_verify_paths();

        return std::make_shared<FoxyClient>(
            net::make_strand(executor_), request, host, port, read_timeout_,
            receiver_, logging_cb_, std::move(ssl_ctx));
    } else {
        std::string port =
            uri_components->has_port() ? uri_components->port() : "80";

        return std::make_shared<FoxyClient>(net::make_strand(executor_),
                                            request, host, port, read_timeout_,
                                            receiver_, logging_cb_);
    }
}

}  // namespace launchdarkly::sse
