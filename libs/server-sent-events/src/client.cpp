#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/use_future.hpp>
#include <foxy/client_session.hpp>
#include <launchdarkly/sse/client.hpp>
#include <launchdarkly/sse/detail/parser.hpp>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/version.hpp>

#include <boost/url/parse.hpp>

#include <chrono>
#include <iostream>
#include <memory>

namespace launchdarkly::sse {

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;  // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

auto const kDefaultUserAgent = BOOST_BEAST_VERSION_STRING;

// Time duration used when no timeout is specified (1 year).
auto const kNoTimeout = std::chrono::hours(8760);

class FoxyClient : public Client,
                   public std::enable_shared_from_this<FoxyClient> {
   public:
    FoxyClient(boost::asio::any_io_executor executor,
               http::request<http::string_body> req,
               std::string host,
               std::string port,
               std::optional<std::chrono::milliseconds> connect_timeout,
               std::optional<std::chrono::milliseconds> read_timeout,
               std::optional<std::chrono::milliseconds> write_timeout,
               Builder::EventReceiver receiver,
               Builder::LogCallback logger,
               net::ssl::context ssl_context)
        : ssl_context_(std::move(ssl_context)),
          host_(std::move(host)),
          port_(std::move(port)),
          connect_timeout_(connect_timeout),
          read_timeout_(read_timeout),
          write_timeout_(read_timeout),
          req_(std::move(req)),
          parser_(),
          session_(executor,
                   foxy::session_opts{
                       .ssl_ctx = *ssl_context_,
                       .timeout = connect_timeout.value_or(kNoTimeout)}),
          logger_(std::move(logger)) {
        parser_.get().body().on_event(std::move(receiver));
    }

    FoxyClient(boost::asio::any_io_executor executor,
               http::request<http::string_body> req,
               std::string host,
               std::string port,
               std::optional<std::chrono::milliseconds> connect_timeout,
               std::optional<std::chrono::milliseconds> read_timeout,
               std::optional<std::chrono::milliseconds> write_timeout,
               Builder::EventReceiver receiver,
               Builder::LogCallback logger)
        : ssl_context_(std::nullopt),
          host_(std::move(host)),
          port_(std::move(port)),
          connect_timeout_(connect_timeout),
          read_timeout_(read_timeout),
          write_timeout_(write_timeout),
          req_(std::move(req)),
          parser_(),
          session_(executor,
                   foxy::session_opts{
                       .timeout = connect_timeout.value_or(kNoTimeout)}),
          logger_(std::move(logger)) {
        parser_.get().body().on_event(std::move(receiver));
    }

    void fail(boost::system::error_code ec, std::string what) {
        logger_("sse-client: " + what + ": " + ec.message());
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

        session_.opts.timeout = read_timeout_.value_or(kNoTimeout);
        session_.async_perpetual_request(
            req_, parser_,
            beast::bind_front_handler(&FoxyClient::on_error,
                                      shared_from_this()));
    }

    void on_error(boost::system::error_code ec) {
        if (ec) {
            return fail(ec, "perpetual request");
        }
    }

    virtual void close() override {
        session_.async_shutdown(boost::asio::use_future).get();
    }

   private:
    std::optional<net::ssl::context> ssl_context_;
    std::string host_;
    std::string port_;
    std::optional<std::chrono::milliseconds> connect_timeout_;
    std::optional<std::chrono::milliseconds> read_timeout_;
    std::optional<std::chrono::milliseconds> write_timeout_;
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
      write_timeout_{std::nullopt},
      connect_timeout_{std::nullopt},
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

Builder& Builder::connect_timeout(std::chrono::milliseconds timeout) {
    connect_timeout_ = timeout;
    return *this;
}

Builder& Builder::read_timeout(std::chrono::milliseconds timeout) {
    read_timeout_ = timeout;
    return *this;
}

Builder& Builder::write_timeout(std::chrono::milliseconds timeout) {
    write_timeout_ = timeout;
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
            net::make_strand(executor_), request, host, port, connect_timeout_,
            read_timeout_, write_timeout_, receiver_, logging_cb_,
            std::move(ssl_ctx));
    } else {
        std::string port =
            uri_components->has_port() ? uri_components->port() : "80";

        return std::make_shared<FoxyClient>(
            net::make_strand(executor_), request, host, port, connect_timeout_,
            read_timeout_, write_timeout_, receiver_, logging_cb_);
    }
}

}  // namespace launchdarkly::sse
