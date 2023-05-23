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

#include <boost/url/parse.hpp>

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
               Builder::EventReceiver receiver,
               Builder::LogCallback logger,
               std::optional<net::ssl::context> maybe_ssl)
        : ssl_context_(std::move(maybe_ssl)),
          host_(std::move(host)),
          port_(std::move(port)),
          connect_timeout_(connect_timeout),
          read_timeout_(read_timeout),
          write_timeout_(write_timeout),
          req_(std::move(req)),
          session_(std::move(executor),
                   launchdarkly::foxy::session_opts{
                       ToOptRef(ssl_context_),
                       connect_timeout.value_or(kNoTimeout)}),
          backoff_(std::chrono::seconds(1), std::chrono::seconds(30)),
          last_event_id_(std::nullopt),
          backoff_timer_(session_.get_executor()),
          event_receiver_(std::move(receiver)),
          logger_(std::move(logger)) {
        create_parser();
    }

    /** The body parser is recreated each time a connection is made because its
     * internal state cannot be explicitly reset.
     *
     * Since SSE body will never end unless
     * an error occurs, the body size limit must be removed.
     */
    void create_parser() {
        body_parser_.emplace();
        body_parser_->body_limit(boost::none);
        body_parser_->get().body().on_event(event_receiver_);
    }

    /**
     * Called whenever the connection needs to be reattempted, triggering
     * a timed wait for the current backoff duration.
     *
     * The body parser's last SSE event ID must be cached so it can be added
     * as a header on the next request (since the parser is destroyed.)
     */
    void do_backoff(std::string const& reason) {
        backoff_.fail();

        std::stringstream msg;
        msg << "backing off in ("
            << std::chrono::duration_cast<std::chrono::seconds>(
                   backoff_.delay())
                   .count()
            << ") seconds due to " << reason;

        logger_(msg.str());

        last_event_id_ = body_parser_->get().body().last_event_id();
        create_parser();
        backoff_timer_.expires_from_now(backoff_.delay());
        backoff_timer_.async_wait(beast::bind_front_handler(
            &FoxyClient::on_backoff, shared_from_this()));
    }

    void on_backoff(boost::system::error_code ec) {
        if (ec == boost::asio::error::operation_aborted) {
            return;
        }
        run();
    }

    void run() override {
        session_.async_connect(
            host_, port_,
            beast::bind_front_handler(&FoxyClient::on_connect,
                                      shared_from_this()));
    }

    void on_connect(boost::system::error_code ec) {
        if (ec == boost::asio::error::operation_aborted) {
            return;
        }
        if (ec) {
            return do_backoff(ec.what());
        }

        if (last_event_id_ && !last_event_id_->empty()) {
            req_.set("last-event-id", *last_event_id_);
        } else {
            req_.erase("last-event-id");
        }
        session_.opts.timeout = write_timeout_.value_or(kNoTimeout);
        session_.async_write(req_,
                             beast::bind_front_handler(&FoxyClient::on_write,
                                                       shared_from_this()));
    }

    void on_write(boost::system::error_code ec, std::size_t amount) {
        boost::ignore_unused(amount);
        if (ec == boost::asio::error::operation_aborted) {
            return;
        }
        if (ec) {
            return do_backoff(ec.what());
        }

        session_.opts.timeout = read_timeout_.value_or(kNoTimeout);
        session_.async_read_header(
            *body_parser_, beast::bind_front_handler(&FoxyClient::on_headers,
                                                     shared_from_this()));
    }

    void on_headers(boost::system::error_code ec, std::size_t amount) {
        boost::ignore_unused(amount);
        if (ec == boost::asio::error::operation_aborted) {
            return;
        }
        if (ec) {
            return do_backoff(ec.what());
        }

        if (!body_parser_->is_header_done()) {
            /* keep reading headers */
            return session_.async_read_header(
                *body_parser_,
                beast::bind_front_handler(&FoxyClient::on_headers,
                                          shared_from_this()));
        }

        /* headers are finished, body is ready */
        auto response = body_parser_->get();
        auto status_class = beast::http::to_status_class(response.result());

        if (status_class == beast::http::status_class::successful) {
            if (!correct_content_type(response)) {
                return do_backoff("invalid Content-Type");
            }

            backoff_.succeed();
            return session_.async_read(
                *body_parser_,
                beast::bind_front_handler(&FoxyClient::on_read_body,
                                          shared_from_this()));
        }

        if (status_class == beast::http::status_class::client_error) {
            if (recoverable_client_error(response.result())) {
                return do_backoff(backoff_reason(response.result()));
            }

            // TODO: error callback

            return;
        }

        do_backoff(backoff_reason(response.result()));
    }

    static std::string backoff_reason(beast::http::status status) {
        std::stringstream ss;
        ss << "HTTP status " << int(status) << " (" << status << ")";
        return ss.str();
    }

    void on_read_body(boost::system::error_code ec, std::size_t amount) {
        boost::ignore_unused(amount);
        if (ec == boost::asio::error::operation_aborted) {
            return;
        }
        do_backoff(ec.what());
    }

    void async_shutdown(std::function<void()> completion) override {
        boost::asio::post(session_.get_executor(),
                          beast::bind_front_handler(&FoxyClient::do_shutdown,
                                                    shared_from_this(),
                                                    std::move(completion)));
    }

    void do_shutdown(std::function<void()> completion) {
        session_.async_shutdown(beast::bind_front_handler(
            &FoxyClient::on_shutdown, std::move(completion)));
    }

    static void on_shutdown(std::function<void()> completion,
                            boost::system::error_code ec) {
        boost::ignore_unused(ec);
        if (completion) {
            completion();
        }
    }

    void fail(boost::system::error_code ec, std::string const& what) {
        logger_("sse-client: " + what + ": " + ec.message());
        async_shutdown(nullptr);
    }

    static bool recoverable_client_error(beast::http::status status) {
        return (status == beast::http::status::bad_request ||
                status == beast::http::status::request_timeout ||
                status == beast::http::status::too_many_requests);
    }

    static bool correct_content_type(FoxyClient::response const& response) {
        if (auto content_type = response.find("content-type");
            content_type != response.end()) {
            return content_type->value().find("text/event-stream") !=
                   content_type->value().npos;
        }
        return false;
    }

   private:
    std::optional<net::ssl::context> ssl_context_;
    std::string host_;
    std::string port_;
    std::optional<std::chrono::milliseconds> connect_timeout_;
    std::optional<std::chrono::milliseconds> read_timeout_;
    std::optional<std::chrono::milliseconds> write_timeout_;
    http::request<http::string_body> req_;
    Builder::EventReceiver event_receiver_;
    std::optional<http::response_parser<body> > body_parser_;
    launchdarkly::foxy::client_session session_;
    std::optional<std::string> last_event_id_;
    Backoff backoff_;
    boost::asio::steady_timer backoff_timer_;
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

    // If this isn't a post or report, ensure the body is empty.
    if (request.method() != http::verb::post &&
        request.method() != http::verb::report) {
        request.body() = "";
    } else {
        // If it is, then setup Content-Type, only if one wasn't
        // specified.
        if (auto content_header = request.find(http::field::content_type);
            content_header == request.end()) {
            request.set(http::field::content_type, "text/plain");
        }
    }

    request.prepare_payload();

    std::string host = uri_components->host();

    request.set(http::field::host, host);
    request.target(uri_components->encoded_target());

    std::string service = uri_components->has_port() ? uri_components->port()
                                                     : uri_components->scheme();

    std::optional<ssl::context> ssl;
    if (service == "https") {
        ssl = launchdarkly::foxy::make_ssl_ctx(ssl::context::tlsv12_client);
        ssl->set_default_verify_paths();
    }

    return std::make_shared<FoxyClient>(
        net::make_strand(executor_), request, host, service, connect_timeout_,
        read_timeout_, write_timeout_, receiver_, logging_cb_, std::move(ssl));
}

}  // namespace launchdarkly::sse
