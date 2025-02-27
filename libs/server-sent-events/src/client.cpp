#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/use_future.hpp>

#include <foxy/client_session.hpp>
#include <launchdarkly/sse/client.hpp>

#include "backoff.hpp"
#include "parser.hpp"

#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/version.hpp>

#include <boost/core/ignore_unused.hpp>

#include <boost/url/parse.hpp>
#include <boost/url/url.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <sstream>

namespace launchdarkly::sse {

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;  // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

auto const kDefaultUserAgent = BOOST_BEAST_VERSION_STRING;

// Time duration used when no timeout is specified (1 year).
auto const kNoTimeout = std::chrono::hours(8760);

// Time duration that the backoff algorithm uses before initiating a new
// connection, the first time a failure is detected.
auto const kDefaultInitialReconnectDelay = std::chrono::seconds(1);

// Maximum duration between backoff attempts.
auto const kDefaultMaxBackoffDelay = std::chrono::seconds(30);

static boost::optional<net::ssl::context&> ToOptRef(
    std::optional<net::ssl::context>& maybe_val) {
    if (maybe_val) {
        return maybe_val.value();
    }
    return boost::none;
}

class FoxyClient : public Client,
                   public std::enable_shared_from_this<FoxyClient> {
   private:
    using cb = std::function<void(launchdarkly::sse::Event)>;
    using body = launchdarkly::sse::detail::EventBody<cb>;
    using response = http::response<body>;

   public:
    FoxyClient(boost::asio::any_io_executor executor,
               http::request<http::string_body> req,
               std::string host,
               std::string port,
               std::optional<std::chrono::milliseconds> connect_timeout,
               std::optional<std::chrono::milliseconds> read_timeout,
               std::optional<std::chrono::milliseconds> write_timeout,
               std::optional<std::chrono::milliseconds> initial_reconnect_delay,
               Builder::EventReceiver receiver,
               Builder::LogCallback logger,
               Builder::ErrorCallback errors,
               std::optional<net::ssl::context> maybe_ssl)
        : ssl_context_(std::move(maybe_ssl)),
          host_(std::move(host)),
          port_(std::move(port)),
          connect_timeout_(connect_timeout),
          read_timeout_(read_timeout),
          write_timeout_(write_timeout),
          req_(std::move(req)),
          event_receiver_(std::move(receiver)),
          logger_(std::move(logger)),
          errors_(std::move(errors)),
          body_parser_(std::nullopt),
          session_(std::nullopt),
          last_event_id_(std::nullopt),
          backoff_(
              initial_reconnect_delay.value_or(kDefaultInitialReconnectDelay),
              kDefaultMaxBackoffDelay),
          backoff_timer_(std::move(executor)),
          last_read_(std::nullopt),
          shutting_down_(false) {
        create_session();
        create_parser();
    }

    // The body parser is recreated each time a connection is made because
    // its internal state cannot be explicitly reset.
    void create_parser() {
        body_parser_.emplace();
        // Remove body read limit because an SSE stream can be infinite.
        body_parser_->body_limit(boost::none);
        body_parser_->get().body().on_event(event_receiver_);
    }

    // The session is recreated each time a connection is made because its
    // internal state cannot be explicitly reset.
    void create_session() {
        session_.emplace(
            backoff_timer_.get_executor(),
            launchdarkly::foxy::session_opts{
                ToOptRef(ssl_context_), connect_timeout_.value_or(kNoTimeout)});
    }

    /**
     * Called whenever the connection needs to be reattempted, triggering
     * a timed wait for the current backoff duration.
     *
     * The body parser's last SSE event ID must be cached so it can be added
     * as a header on the next request (since the parser is destroyed.)
     */
    void async_backoff(std::string const& reason) {
        backoff_.fail();

        if (auto id = body_parser_->get().body().last_event_id()) {
            if (!id->empty()) {
                last_event_id_ = id;
            }
        }

        std::stringstream msg;
        msg << "backing off in ("
            << std::chrono::duration_cast<std::chrono::seconds>(
                   backoff_.delay())
                   .count()
            << ") seconds due to " << reason;

        logger_(msg.str());

        create_session();
        create_parser();
        backoff_timer_.expires_after(backoff_.delay());
        backoff_timer_.async_wait(beast::bind_front_handler(
            &FoxyClient::on_backoff, shared_from_this()));
    }

    void on_backoff(boost::system::error_code ec) {
        if (ec == boost::asio::error::operation_aborted) {
            return;
        }
        do_run();
    }

    void async_connect() override {
        boost::asio::post(
            session_->get_executor(),
            beast::bind_front_handler(&FoxyClient::do_run, shared_from_this()));
    }

    void do_run() {
        session_->async_connect(
            host_, port_,
            beast::bind_front_handler(&FoxyClient::on_connect,
                                      shared_from_this()));
    }

    void on_connect(boost::system::error_code ec) {
        if (ec == boost::asio::error::operation_aborted) {
            return;
        }
        if (ec) {
            return async_backoff(ec.what());
        }

        if (last_event_id_) {
            req_.set("last-event-id", *last_event_id_);
        } else {
            req_.erase("last-event-id");
        }
        session_->opts.timeout = write_timeout_.value_or(kNoTimeout);
        session_->async_write(req_,
                              beast::bind_front_handler(&FoxyClient::on_write,
                                                        shared_from_this()));
    }

    void on_write(boost::system::error_code ec, std::size_t amount) {
        boost::ignore_unused(amount);
        if (ec == boost::asio::error::operation_aborted) {
            return;
        }
        if (ec) {
            return async_backoff(ec.what());
        }

        session_->opts.timeout = read_timeout_.value_or(kNoTimeout);
        session_->async_read_header(
            *body_parser_, beast::bind_front_handler(&FoxyClient::on_headers,
                                                     shared_from_this()));
    }

    void on_headers(boost::system::error_code ec, std::size_t amount) {
        boost::ignore_unused(amount);
        if (ec == boost::asio::error::operation_aborted) {
            return;
        }
        if (ec) {
            return async_backoff(ec.what());
        }

        if (!body_parser_->is_header_done()) {
            /* keep reading headers */
            return session_->async_read_header(
                *body_parser_,
                beast::bind_front_handler(&FoxyClient::on_headers,
                                          shared_from_this()));
        }

        /* headers are finished, body is ready */
        auto response = body_parser_->get();
        auto status_class = beast::http::to_status_class(response.result());

        if (status_class == beast::http::status_class::successful) {
            if (response.result() == beast::http::status::no_content) {
                errors_(
                    errors::UnrecoverableClientError{http::status::no_content});
                return;
            }
            if (!correct_content_type(response)) {
                return async_backoff("invalid Content-Type");
            }

            logger_("connected");
            backoff_.succeed();

            last_read_ = std::chrono::steady_clock::now();
            return session_->async_read_some(
                *body_parser_,
                beast::bind_front_handler(&FoxyClient::on_read_body,
                                          shared_from_this()));
        }

        if (status_class == beast::http::status_class::redirection) {
            if (can_redirect(response)) {
                auto const& location_header =
                    response.find("location")->value();

                auto new_url = redirect_url("base", location_header);

                if (!new_url) {
                    errors_(errors::InvalidRedirectLocation{location_header});
                    return;
                }

                req_.set(http::field::host, new_url->host());
                req_.target(new_url->encoded_target());
            } else {
                errors_(errors::NotRedirectable{});
                return;
            }
        }

        if (status_class == beast::http::status_class::client_error) {
            if (recoverable_client_error(response.result())) {
                return async_backoff(backoff_reason(response.result()));
            }

            errors_(errors::UnrecoverableClientError{response.result()});
            return;
        }

        async_backoff(backoff_reason(response.result()));
    }

    static std::string backoff_reason(beast::http::status status) {
        std::stringstream ss;
        ss << "HTTP status " << int(status) << " (" << status << ")";
        return ss.str();
    }

    void on_read_body(boost::system::error_code ec, std::size_t amount) {
        boost::ignore_unused(amount);
        if (ec) {
            if (shutting_down_) {
                return;
            }
            if (ec == boost::asio::error::operation_aborted) {
                errors_(errors::ReadTimeout{read_timeout_});
                return async_backoff(
                    "aborting read of response body (timeout)");
            }
            return async_backoff(ec.what());
        }

        // The server can indicate that the chunk encoded response is done
        // by sending a final chunk + CRLF. The body parser will have
        // detected this.
        if (body_parser_->is_done()) {
            return async_backoff("receiving final chunk");
        }

        log_and_update_last_read(amount);
        return session_->async_read_some(
            *body_parser_, beast::bind_front_handler(&FoxyClient::on_read_body,
                                                     shared_from_this()));
    }

    /** Logs a message indicating that an async_read_some operation
     * on the response's body has completed, and how long that operation took
     * (using a monotonic clock.)
     *
     * This is useful for debugging timeout-related issues, like receiving a
     * heartbeat message.
     */
    void log_and_update_last_read(std::size_t amount) {
        if (last_read_) {
            auto sec_since_last_read =
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - *last_read_)
                    .count();
            logger_("read (" + std::to_string(amount) + ") bytes in (" +
                    std::to_string(sec_since_last_read) + ") sec");
        } else {
            logger_("read (" + std::to_string(amount) + ") bytes");
        }
        last_read_ = std::chrono::steady_clock::now();
    }

    void async_shutdown(std::function<void()> completion) override {
        // Get on the session's executor, otherwise the code in the completion
        // handler could race.
        boost::asio::post(session_->get_executor(),
                          beast::bind_front_handler(&FoxyClient::do_shutdown,
                                                    shared_from_this(),
                                                    std::move(completion)));
    }

    void do_shutdown(std::function<void()> completion) {
        // Signal to the body reader operation that if it completes,
        // it should return instead of starting another async read.
        shutting_down_ = true;
        // If any backoff is taking place, cancel that as well.
        backoff_timer_.cancel();

        try {
            // Cancels the outstanding read.
            if (session_->stream.is_ssl()) {
                session_->stream.ssl().next_layer().cancel();
            } else {
                session_->stream.plain().cancel();
            }
        } catch (boost::system::system_error const& err) {
            // The stream is likely already closed. Potentially the network
            // interface that was associated with the stream is no longer
            // available.
            logger_("exception closing stream: " + std::string(err.what()));
        }

        // Ideally we would call session_->async_shutdown() here to gracefully
        // terminate the SSL session. For unknown reasons, this call appears to
        // hang indefinitely and never complete until the SDK client is
        // destroyed.
        //
        // A workaround is to set a timeout on the operation, say 1 second. This
        // gives the opportunity to shutdown gracefully and then if that fails,
        // we could close the socket directly. But that also doesn't seem to
        // work: even with the timeout, the operation still doesn't complete.
        //
        // So the most robust solution appears to be closing the socket
        // directly. This is not ideal because it doesn't send a close_notify to
        // the server.
        boost::system::error_code ec;
        session_->stream.plain().shutdown(
            boost::asio::ip::tcp::socket::shutdown_both, ec);
        session_->stream.plain().close(ec);

        if (completion) {
            completion();
        }
    }

    // Some client errors are considered recoverable, meaning they could be
    // resolved by reconnecting. Returns true if the status is one of those.
    static bool recoverable_client_error(beast::http::status status) {
        return (status == beast::http::status::bad_request ||
                status == beast::http::status::request_timeout ||
                status == beast::http::status::too_many_requests);
    }

    // A naive comparison of content-type to 'text/event-stream' is incorrect
    // because multiple content types may be present.
    static bool correct_content_type(FoxyClient::response const& response) {
        if (auto content_type = response.find("content-type");
            content_type != response.end()) {
            return content_type->value().find("text/event-stream") !=
                   content_type->value().npos;
        }
        return false;
    }

    // If the server redirects, ensure the location header is present.
    static bool can_redirect(FoxyClient::response const& response) {
        return (response.result() == beast::http::status::moved_permanently ||
                response.result() == beast::http::status::temporary_redirect) &&
               response.find("location") != response.end();
    }

    // Generates a redirect URL from a base and provided location. Since the
    // location might not be absolute, it may be necessary to resolve it against
    // the base.
    static std::optional<boost::urls::url> redirect_url(
        std::string orig_base,
        std::string orig_location) {
        auto location = boost::urls::parse_uri(orig_location);
        if (!location) {
            return std::nullopt;
        }
        if (location->has_scheme()) {
            return location.value();
        }

        boost::urls::url base(orig_base);
        auto result = base.resolve(*location);
        if (!result) {
            return std::nullopt;
        }

        return base;
    }

   private:
    // Optional, but necessary for establishing TLS connections. If it is
    // omitted and the scheme is https://, the connection will fail. Can be
    // reused across connections.
    std::optional<net::ssl::context> ssl_context_;

    std::string host_;
    std::string port_;

    // If present, the max amount of time that can be spent resolving DNS
    // and setting up the initial connection. This doesn't include writing the
    // request nor receiving the response.
    std::optional<std::chrono::milliseconds> connect_timeout_;

    // If present, the max amount of time that can be spent after reading bytes
    // before more bytes must be read. Applies after sending the request, but
    // before receiving the response headers. Intended to terminate connections
    // where the sender has hung up. LaunchDarkly sends a heartbeat every 180
    // seconds, so the read timeout must be greater than this.
    std::optional<std::chrono::milliseconds> read_timeout_;

    // If present, the max amount of time that can be spent sending the request.
    std::optional<std::chrono::milliseconds> write_timeout_;

    // The request sent to the server, which persists across reconnection
    // attempts.
    http::request<http::string_body> req_;

    // Callback which executed whenever an SSE event is received. This is
    // passed into the body_parser whenever it is constructed, so it needs to be
    // stored here for future use.
    Builder::EventReceiver event_receiver_;

    // Callback executed when log messages must be generated. The provider of
    // the callback dictates the log level, which at the moment of writing is
    // 'debug'.
    Builder::LogCallback logger_;

    // Callback executed to report errors. This is the primary mechanism by
    // which the client communicates error conditions to the user.
    Builder::ErrorCallback errors_;

    // Customized parser (see parser.hpp) which repeatedly receives chunks of
    // data and parses them into SSE events. It cannot be reused across
    // connections, hence the optional so it can be destroyed easily.
    std::optional<http::response_parser<body>> body_parser_;

    // The primary network primitive used to read/write to the server. It is our
    // responsibility to ensure it is called from only one thread at a time. It
    // cannot be reused across connections, similar to the body_parser.
    std::optional<launchdarkly::foxy::client_session> session_;

    // Stores the last known SSE event ID, which can be provided to the server
    // upon reconnection.
    std::optional<std::string> last_event_id_;

    // Computes the backoff delays used whenever the connection drops
    // unnaturally.
    Backoff backoff_;

    // Used in concert with the backoff member to delay reconnection attempts.
    // The timer must be cancelled when shutdown is requested, because an
    // outstanding backoff delay might be in progress.
    boost::asio::steady_timer backoff_timer_;

    // Stores the timestamp of the last read in order to augment debug logging.
    std::optional<std::chrono::steady_clock::time_point> last_read_;

    // Upon completing a read, a new async read is generally initiated. This
    // determines whether to return and thus end the operation gracefully (true)
    // or to perform backoff (false).
    bool shutting_down_;
};

Builder::Builder(net::any_io_executor ctx, std::string url)
    : url_{std::move(url)},
      http_proxy_{std::nullopt},
      executor_{std::move(ctx)},
      read_timeout_{std::nullopt},
      write_timeout_{std::nullopt},
      connect_timeout_{std::nullopt},
      initial_reconnect_delay_{std::nullopt},
      logging_cb_([](auto msg) {}),
      receiver_([](launchdarkly::sse::Event const&) {}),
      error_cb_([](auto err) {}),
      skip_verify_peer_(false),
      custom_ca_file_(std::nullopt) {
    request_.version(11);
    request_.set(http::field::user_agent, kDefaultUserAgent);
    request_.method(http::verb::get);
    request_.set(http::field::accept, "text/event-stream");
    request_.set(http::field::cache_control, "no-cache");
}

Builder& Builder::http_proxy(std::string proxy) {
    http_proxy_ = std::move(proxy);
    return *this;
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

Builder& Builder::initial_reconnect_delay(std::chrono::milliseconds delay) {
    initial_reconnect_delay_ = delay;
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

Builder& Builder::logger(LogCallback callback) {
    logging_cb_ = std::move(callback);
    return *this;
}

Builder& Builder::errors(ErrorCallback callback) {
    error_cb_ = std::move(callback);
    return *this;
}

Builder& Builder::skip_verify_peer(bool skip_verify_peer) {
    skip_verify_peer_ = skip_verify_peer;
    return *this;
}

Builder& Builder::custom_ca_file(std::string path) {
    if (path.empty()) {
        custom_ca_file_ = std::nullopt;
        return *this;
    }
    custom_ca_file_ = std::move(path);
    return *this;
}

// Returns the host with a port appended, if the port is not 80 or 443.
// Although not strictly necessary, it is meant to match what browsers do.
std::string host_with_optional_port(
    boost::system::result<boost::urls::url_view> const& url) {
    if (url->has_port() && url->port() != "80" && url->port() != "443") {
        std::string host = url->host();
        host.append(":");
        host.append(url->port());
        return host;
    }
    return url->host();
}

bool validate_scheme(boost::system::result<boost::urls::url_view> const& url) {
    if (!url->has_scheme()) {
        return false;
    }
    return url->scheme_id() == boost::urls::scheme::http ||
           url->scheme_id() == boost::urls::scheme::https;
}

std::shared_ptr<Client> Builder::build() {
    auto uri_components = boost::urls::parse_uri(url_);
    if (!uri_components) {
        return nullptr;
    }

    if (!validate_scheme(uri_components)) {
        return nullptr;
    }

    auto request = request_;

    if (request.method() == http::verb::post ||
        request.method() == http::verb::report) {
        // If this is a post or report, set the content type (if not already
        // specified.)
        if (auto const content_header = request.find(http::field::content_type);
            content_header == request.end()) {
            request.set(http::field::content_type, "text/plain");
        }
    } else {
        // If this is *not* a post or report, then make sure no body is sent.
        request.body() = "";
    }

    request.prepare_payload();

    request.set(http::field::host, host_with_optional_port(uri_components));
    request.target(uri_components->encoded_target());

    // We need to open a TCP socket to a host and port. The resolver takes
    // either a port number, or a "service name". So if the uri has a port
    // number, we'll use that - otherwise, we'll assume the service name is the
    // scheme.

    std::string tcp_host = uri_components->host();
    std::string tcp_port = uri_components->has_port()
                               ? uri_components->port()
                               : uri_components->scheme();

    if (http_proxy_) {
        auto http_proxy_components = boost::urls::parse_uri(*http_proxy_);
        if (!http_proxy_components) {
            return nullptr;
        }

        // An HTTP proxy talks http to the client. Setting https scheme
        // indicates a misconfiguration.
        if (http_proxy_components->scheme_id() != boost::urls::scheme::http) {
            return nullptr;
        }

        tcp_host = http_proxy_components->host();

        // If they didn't specify a port, use the scheme as the service
        //  (which we've enforced to be http above.)
        tcp_port = http_proxy_components->has_port()
                       ? http_proxy_components->port()
                       : http_proxy_components->scheme();

        request.target(url_);
    }

    std::optional<ssl::context> ssl;
    if (!http_proxy_ &&
        uri_components->scheme_id() == boost::urls::scheme::https) {
        ssl = launchdarkly::foxy::make_ssl_ctx(ssl::context::tlsv12_client);

        ssl->set_default_verify_paths();
        ssl->set_verify_mode(ssl::context::verify_peer);

        if (custom_ca_file_) {
            assert(!custom_ca_file_->empty());
            ssl->load_verify_file(*custom_ca_file_);
        }

        if (skip_verify_peer_) {
            ssl->set_verify_mode(ssl::context::verify_none);
        }
    }

    return std::make_shared<FoxyClient>(net::make_strand(executor_), request,
                                        tcp_host, tcp_port, connect_timeout_,
                                        read_timeout_, write_timeout_,
                                        initial_reconnect_delay_, receiver_,
                                        logging_cb_, error_cb_, std::move(ssl));
}

}  // namespace launchdarkly::sse
