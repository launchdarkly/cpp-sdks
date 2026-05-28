#ifdef LD_CURL_NETWORKING

#include "curl_client.hpp"

#include <boost/asio/post.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/url/url.hpp>

#include <charconv>
#include <sstream>
#include <system_error>

namespace launchdarkly::sse {
namespace beast = boost::beast;
namespace http = beast::http;

// Time duration used when no timeout is specified (1 year).
auto const kNoTimeout = std::chrono::hours(8760);

// Time duration that the backoff algorithm uses before initiating a new
// connection, the first time a failure is detected.
auto const kDefaultInitialReconnectDelay = std::chrono::seconds(1);

// Maximum duration between backoff attempts.
auto const kDefaultMaxBackoffDelay = std::chrono::seconds(30);

constexpr auto kCurlTransferContinue = 0;
constexpr auto kCurlTransferAbort = 1;

constexpr auto kHttpStatusCodeMovedPermanently = 301;
constexpr auto kHttpStatusCodeTemporaryRedirect = 307;

CurlClient::CurlClient(
    boost::asio::any_io_executor executor,
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
    Builder::ConnectionHook connection_hook,
    Builder::ResponseHook response_hook,
    bool skip_verify_peer,
    std::optional<std::string> custom_ca_file,
    bool use_https,
    std::optional<std::string> proxy_url)
    : host_(std::move(host)),
      port_(std::move(port)),
      event_receiver_(std::move(receiver)),
      logger_(std::move(logger)),
      errors_(std::move(errors)),
      connection_hook_(std::move(connection_hook)),
      response_hook_(std::move(response_hook)),
      use_https_(use_https),
      backoff_timer_(executor),
      multi_manager_(CurlMultiManager::create(executor)),
      backoff_(initial_reconnect_delay.value_or(kDefaultInitialReconnectDelay),
               kDefaultMaxBackoffDelay) {
    request_context_ = std::make_shared<RequestContext>(
        build_url(req), std::move(req), connect_timeout, read_timeout,
        write_timeout, std::move(custom_ca_file), std::move(proxy_url),
        skip_verify_peer);
}

CurlClient::~CurlClient() {
    request_context_->shutdown();
    backoff_timer_.cancel();
}

void CurlClient::async_connect() {
    boost::asio::post(backoff_timer_.get_executor(),
                      [self = shared_from_this()]() { self->do_run(); });
}

void CurlClient::async_restart(std::string const& reason) {
    boost::asio::post(backoff_timer_.get_executor(), [self = shared_from_this(),
                                                      reason]() {
        // Close the socket to abort the current transfer.
        // CURL will detect the error and call the completion
        // handler, which will trigger backoff and reconnection.
        self->log_message("async_restart: aborting transfer due to " + reason);
        self->request_context_->abort_transfer();
    });
}

void CurlClient::do_run() {
    if (request_context_->is_shutting_down()) {
        return;
    }

    if (connection_hook_) {
        connection_hook_(&request_context_->req);
        request_context_->url = build_url(request_context_->req);
    }

    // Set Last-Event-ID if we have one from a previous connection, otherwise
    // erase it to override any value set by the hook.
    if (request_context_->last_event_id &&
        !request_context_->last_event_id->empty()) {
        request_context_->req.set("last-event-id",
                                  *request_context_->last_event_id);
    } else {
        request_context_->req.erase("last-event-id");
    }

    request_context_->req.prepare_payload();

    auto ctx = request_context_;
    auto weak_self = weak_from_this();
    std::weak_ptr<RequestContext> weak_ctx = ctx;
    ctx->set_callbacks(Callbacks(
        [weak_self, weak_ctx](std::string const& message) {
            if (auto ctx = weak_ctx.lock()) {
                if (auto self = weak_self.lock()) {
                    boost::asio::post(
                        self->backoff_timer_.get_executor(),
                        [self, message]() { self->async_backoff(message); });
                }
            }
        },
        [weak_self, weak_ctx](Event const& event) {
            if (auto ctx = weak_ctx.lock()) {
                if (auto self = weak_self.lock()) {
                    boost::asio::post(
                        self->backoff_timer_.get_executor(),
                        [self, event]() { self->event_receiver_(event); });
                }
            }
        },
        [weak_self, weak_ctx](Error const& error) {
            if (auto ctx = weak_ctx.lock()) {
                if (auto const self = weak_self.lock()) {
                    // report_error does an asio post.
                    self->report_error(error);
                }
            }
        },
        [weak_self, weak_ctx]() {
            if (auto ctx = weak_ctx.lock()) {
                if (auto const self = weak_self.lock()) {
                    boost::asio::post(self->backoff_timer_.get_executor(),
                                      [self]() { self->backoff_.succeed(); });
                }
            }
        },
        [weak_self, weak_ctx](std::string const& message) {
            if (auto ctx = weak_ctx.lock()) {
                if (auto const self = weak_self.lock()) {
                    self->log_message(message);
                }
            }
        },
        [weak_self, weak_ctx](http::response_header<> headers) {
            if (auto ctx = weak_ctx.lock()) {
                if (auto const self = weak_self.lock()) {
                    if (self->response_hook_) {
                        boost::asio::post(
                            self->backoff_timer_.get_executor(),
                            [self, headers = std::move(headers)]() {
                                self->response_hook_(headers);
                            });
                    }
                }
            }
        }));
    // Start request using CURL multi (non-blocking)
    PerformRequestWithMulti(multi_manager_, ctx);
}

void CurlClient::async_backoff(std::string const& reason) {
    backoff_.fail();

    std::stringstream msg;
    msg << "backing off in ("
        << std::chrono::duration_cast<std::chrono::seconds>(backoff_.delay())
               .count()
        << ") seconds due to " << reason;

    log_message(msg.str());

    auto weak_self = weak_from_this();
    backoff_timer_.expires_after(backoff_.delay());
    backoff_timer_.async_wait([weak_self](boost::system::error_code const& ec) {
        if (auto self = weak_self.lock()) {
            self->on_backoff(ec);
        }
    });
}

void CurlClient::on_backoff(boost::system::error_code const& ec) {
    if (ec || request_context_->is_shutting_down()) {
        return;
    }
    do_run();
}

std::string CurlClient::build_url(
    http::request<http::string_body> const& req) const {
    std::string const scheme = use_https_ ? "https" : "http";

    std::string url = scheme + "://" + host_;

    // Add port if it's not the default service name
    // port_ can be either a port number (like "8123") or service name (like
    // "https"/"http")
    if (port_ != "https" && port_ != "http") {
        url += ":" + port_;
    }

    url += std::string(req.target());

    return url;
}

bool CurlClient::SetupCurlOptions(CURL* curl,
                                  curl_slist** out_headers,
                                  RequestContext& context) {
    // Helper macro to check curl_easy_setopt return values
    // Returns false on error to signal setup failure
#define CURL_SETOPT_CHECK(handle, option, parameter)                          \
    do {                                                                      \
        CURLcode code = curl_easy_setopt(handle, option, parameter);          \
        if (code != CURLE_OK) {                                               \
            context.log_message("curl_easy_setopt failed for " #option ": " + \
                                std::string(curl_easy_strerror(code)));       \
            return false;                                                     \
        }                                                                     \
    } while (0)

    // Set URL
    CURL_SETOPT_CHECK(curl, CURLOPT_URL, context.url.c_str());

    // Set HTTP method
    switch (context.req.method()) {
        case http::verb::get:
            CURL_SETOPT_CHECK(curl, CURLOPT_HTTPGET, 1L);
            break;
        case http::verb::post:
            CURL_SETOPT_CHECK(curl, CURLOPT_POST, 1L);
            break;
        case http::verb::report:
            CURL_SETOPT_CHECK(curl, CURLOPT_CUSTOMREQUEST, "REPORT");
            break;
        default:
            CURL_SETOPT_CHECK(curl, CURLOPT_HTTPGET, 1L);
            break;
    }

    // Set request body if present
    if (!context.req.body().empty()) {
        CURL_SETOPT_CHECK(curl, CURLOPT_POSTFIELDS, context.req.body().c_str());
        CURL_SETOPT_CHECK(curl, CURLOPT_POSTFIELDSIZE,
                          context.req.body().size());
    }

    // Set headers
    struct curl_slist* headers = nullptr;
    for (auto const& field : context.req) {
        std::string header = std::string(field.name_string()) + ": " +
                             std::string(field.value());
        headers = curl_slist_append(headers, header.c_str());
    }

    if (headers) {
        CURL_SETOPT_CHECK(curl, CURLOPT_HTTPHEADER, headers);
    }

    // Set timeouts with millisecond precision
    if (context.connect_timeout) {
        CURL_SETOPT_CHECK(curl, CURLOPT_CONNECTTIMEOUT_MS,
                          context.connect_timeout->count());
    }

    // For read timeout, use progress callback
    if (context.read_timeout) {
        context.last_progress_time = std::chrono::steady_clock::now();
        context.last_download_amount = 0;
        CURL_SETOPT_CHECK(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
        CURL_SETOPT_CHECK(curl, CURLOPT_XFERINFODATA, &context);
        CURL_SETOPT_CHECK(curl, CURLOPT_NOPROGRESS, 0L);
    }

    // Set TLS options
    if (context.skip_verify_peer) {
        CURL_SETOPT_CHECK(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        CURL_SETOPT_CHECK(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    } else {
        CURL_SETOPT_CHECK(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        CURL_SETOPT_CHECK(curl, CURLOPT_SSL_VERIFYHOST, 2L);

        if (context.custom_ca_file) {
            CURL_SETOPT_CHECK(curl, CURLOPT_CAINFO,
                              context.custom_ca_file->c_str());
        }
    }

    // Set proxy if configured
    // When proxy_url_ is set, it takes precedence over environment variables.
    // Empty string explicitly disables proxy (overrides environment variables).
    if (context.proxy_url) {
        CURL_SETOPT_CHECK(curl, CURLOPT_PROXY, context.proxy_url->c_str());
    }
    // If proxy_url_ is std::nullopt, CURL will use environment variables
    // (default behavior)

    // Set callbacks
    CURL_SETOPT_CHECK(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    CURL_SETOPT_CHECK(curl, CURLOPT_WRITEDATA, &context);
    CURL_SETOPT_CHECK(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
    CURL_SETOPT_CHECK(curl, CURLOPT_HEADERDATA, &context);
    CURL_SETOPT_CHECK(curl, CURLOPT_OPENSOCKETFUNCTION, OpenSocketCallback);
    CURL_SETOPT_CHECK(curl, CURLOPT_OPENSOCKETDATA, &context);

    // Follow redirects
    CURL_SETOPT_CHECK(curl, CURLOPT_FOLLOWLOCATION, 1L);
    CURL_SETOPT_CHECK(curl, CURLOPT_MAXREDIRS, 20L);

#undef CURL_SETOPT_CHECK

    *out_headers = headers;
    return true;
}

// Handle CURL progress.
//
// https://curl.se/libcurl/c/CURLOPT_XFERINFOFUNCTION.html
int CurlClient::ProgressCallback(void* clientp,
                                 curl_off_t dltotal,
                                 curl_off_t dlnow,
                                 curl_off_t ultotal,
                                 curl_off_t ulnow) {
    auto* context = static_cast<RequestContext*>(clientp);

    if (context->is_shutting_down()) {
        return kCurlTransferAbort;
    }

    // Check if we've exceeded the read timeout
    if (context->read_timeout) {
        auto const now = std::chrono::steady_clock::now();

        // If download amount has changed, update the last progress time
        if (dlnow != context->last_download_amount) {
            context->last_download_amount = dlnow;
            context->last_progress_time = now;
        } else {
            // No new data - check if we've exceeded the timeout
            auto const elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - context->last_progress_time);

            if (elapsed > *context->read_timeout) {
                return kCurlTransferAbort;
            }
        }
    }

    return kCurlTransferContinue;
}

// Handle the curl socket opening.
//
// https://curl.se/libcurl/c/CURLOPT_OPENSOCKETFUNCTION.html
curl_socket_t CurlClient::OpenSocketCallback(void* clientp,
                                             curlsocktype purpose,
                                             curl_sockaddr const* address) {
    auto* context = static_cast<RequestContext*>(clientp);

    // Create the socket
    curl_socket_t sockfd =
        socket(address->family, address->socktype, address->protocol);

    // Store it so we can close it during shutdown
    if (sockfd != CURL_SOCKET_BAD) {
        context->set_curl_socket(sockfd);
    }

    return sockfd;
}

// Callback for writing response data
//
// https://curl.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
size_t CurlClient::WriteCallback(char const* data,
                                 size_t size,
                                 size_t nmemb,
                                 void* userp) {
    size_t total_size = size * nmemb;
    auto* context = static_cast<RequestContext*>(userp);

    if (context->is_shutting_down()) {
        return 0;  // Abort the transfer
    }

    // Set up the event receiver callback for the parser
    context->parser_body->on_event([context](Event event) {
        // Track last event ID for reconnection
        if (event.id()) {
            context->last_event_id = event.id();
        }
        context->receive(std::move(event));
    });

    std::string_view const data_view(data, total_size);
    context->parser_reader->put(data_view);

    return total_size;
}

// Callback for reading request headers
//
// https://curl.se/libcurl/c/CURLOPT_HEADERFUNCTION.html
//
// libcurl invokes this once per CRLF-terminated response line: the HTTP status
// line, then each header, then an empty terminator. With
// CURLOPT_FOLLOWLOCATION enabled the cycle repeats for each response in the
// redirect chain.
size_t CurlClient::HeaderCallback(char const* buffer,
                                  size_t size,
                                  size_t nitems,
                                  void* userdata) {
    size_t const total_size = size * nitems;
    auto* context = static_cast<RequestContext*>(userdata);

    std::string_view line(buffer, total_size);
    // Strip the line terminator. Allow bare LF or bare CR per RFC 9112 §2.2;
    // libcurl preserves the original wire bytes for HTTP/1.x (only HTTP/2
    // synthesizes CRLF), so a non-compliant origin can deliver bare LF here.
    while (!line.empty() &&
           (line.back() == '\r' || line.back() == '\n')) {
        line.remove_suffix(1);
    }

    if (line.empty()) {
        // Terminator. If we're between responses (e.g., the line ends a
        // chunked-transfer trailer block), there's nothing to emit.
        if (context->reading_headers) {
            context->response(std::move(context->current_response));
            context->current_response = http::response_header<>{};
            context->reading_headers = false;
        }
        return total_size;
    }

    if (line.substr(0, 5) == "HTTP/") {
        // Status line: "HTTP/X.Y CODE REASON". Only legitimate before any
        // header has been seen for this response — an interior HTTP/ line
        // would otherwise wipe accumulated state.
        if (context->reading_headers) {
            return total_size;
        }
        // Beast default-constructs result_ to status::ok (200); reset to 0
        // so an unparseable status line surfaces as result_int() == 0.
        context->current_response = http::response_header<>{};
        context->current_response.result(0);
        auto const code_start = line.find(' ');
        if (code_start != std::string_view::npos) {
            unsigned code = 0;
            auto const result = std::from_chars(
                line.data() + code_start + 1, line.data() + line.size(), code);
            // Three-digit status per RFC 7231 §6; the tight bound avoids
            // result(unsigned) throwing across the libcurl C frame.
            if (result.ec == std::errc{} && code >= 100 && code <= 999) {
                context->current_response.result(code);
            }
        }
        context->reading_headers = true;
        return total_size;
    }

    if (!context->reading_headers) {
        // Header line received outside an active response — chunked trailer
        // or protocol-level junk. Ignore.
        return total_size;
    }

    auto const colon = line.find(':');
    if (colon != std::string_view::npos) {
        std::string_view name = line.substr(0, colon);
        // HTTP optional whitespace (OWS) per RFC 7230 §3.2.3 is SP or HTAB.
        std::string_view value = line.substr(colon + 1);
        while (!value.empty() &&
               (value.front() == ' ' || value.front() == '\t')) {
            value.remove_prefix(1);
        }
        while (!value.empty() &&
               (value.back() == ' ' || value.back() == '\t')) {
            value.remove_suffix(1);
        }
        // insert() preserves duplicate-name headers (Set-Cookie, Via, …);
        // set() would collapse them and diverge from the Foxy backend.
        context->current_response.insert(std::string(name), std::string(value));

        if (beast::iequals(name, "Content-Type") &&
            value.find("text/event-stream") == std::string_view::npos) {
            context->log_message("warning: unexpected Content-Type: " +
                                 std::string(line));
        }
    }

    return total_size;
}

void CurlClient::PerformRequestWithMulti(
    std::shared_ptr<CurlMultiManager> multi_manager,
    std::shared_ptr<RequestContext> context) {
    if (context->is_shutting_down()) {
        return;
    }

    // Initialize parser for new connection (last_event_id is tracked
    // separately)
    context->init_parser();

    std::shared_ptr<CURL> curl(curl_easy_init(), curl_easy_cleanup);
    if (!curl) {
        if (context->is_shutting_down()) {
            return;
        }

        context->backoff("failed to initialize CURL");
        return;
    }

    curl_slist* headers = nullptr;
    if (!SetupCurlOptions(curl.get(), &headers, *context)) {
        // setup_curl_options returned false, indicating an error (it already
        // logged the error)

        if (context->is_shutting_down()) {
            return;
        }

        context->backoff("failed to set CURL options");
        return;
    }

    // Add handle to multi manager for async processing
    // Headers will be freed automatically by CurlMultiManager
    std::weak_ptr<RequestContext> weak_context = context;
    multi_manager->add_handle(
        curl, headers,
        [weak_context](std::shared_ptr<CURL> easy,
                       CurlMultiManager::Result result) {
            auto context = weak_context.lock();
            if (!context) {
                return;
            }

            // Check if this was a read timeout from the multi manager
            if (result.type == CurlMultiManager::Result::Type::ReadTimeout) {
                if (!context->is_shutting_down()) {
                    context->error(errors::ReadTimeout{context->read_timeout});
                    context->backoff("read timeout - no data received");
                }
                return;
            }

            // Handle CURLcode result
            CURLcode res = result.curl_code;

            // Get response code
            long response_code = 0;
            curl_easy_getinfo(easy.get(), CURLINFO_RESPONSE_CODE,
                              &response_code);

            // Handle HTTP status codes
            auto status = static_cast<http::status>(response_code);
            auto status_class = http::to_status_class(status);

            if (context->is_shutting_down()) {
                return;
            }

            if (status_class == http::status_class::redirection) {
                // The internal CURL handling of redirects failed.
                // This situation is likely the result of a missing redirect
                // header or empty header.
                context->error(errors::NotRedirectable{});
                return;
            }

            // Handle result
            if (res != CURLE_OK) {
                if (context->is_shutting_down()) {
                    return;
                }

                // Check if the error was due to progress callback aborting
                // (read timeout)
                if (res == CURLE_ABORTED_BY_CALLBACK && context->read_timeout) {
                    context->error(errors::ReadTimeout{context->read_timeout});
                    context->backoff(
                        "aborting read of response body (timeout)");
                } else {
                    std::string error_msg =
                        "CURL error: " + std::string(curl_easy_strerror(res));
                    context->backoff(error_msg);
                }

                return;
            }

            if (status_class == http::status_class::successful) {
                if (status == http::status::no_content) {
                    if (!context->is_shutting_down()) {
                        context->error(errors::UnrecoverableClientError{
                            http::status::no_content});
                    }
                    return;
                }
                context->reset_backoff();
                // Connection ended normally, reconnect
                if (!context->is_shutting_down()) {
                    context->backoff("connection closed normally");
                }
                return;
            }

            if (status_class == http::status_class::client_error) {
                if (!context->is_shutting_down()) {
                    bool recoverable =
                        (status == http::status::bad_request ||
                         status == http::status::request_timeout ||
                         status == http::status::too_many_requests);

                    if (recoverable) {
                        std::stringstream ss;
                        ss << "HTTP status " << static_cast<int>(status);
                        context->backoff(ss.str());
                    } else {
                        context->error(
                            errors::UnrecoverableClientError{status});
                    }
                }
                return;
            }

            // Server error or other - backoff and retry
            if (!context->is_shutting_down()) {
                std::stringstream ss;
                ss << "HTTP status " << static_cast<int>(status);
                context->backoff(ss.str());
            }
        },
        context->read_timeout);
}

void CurlClient::async_shutdown(std::function<void()> completion) {
    boost::asio::post(
        backoff_timer_.get_executor(),
        [self = shared_from_this(), completion = std::move(completion)]() {
            self->do_shutdown(completion);
        });
}

void CurlClient::do_shutdown(std::function<void()> const& completion) {
    request_context_->shutdown();
    backoff_timer_.cancel();

    if (completion) {
        completion();
    }
}

void CurlClient::log_message(std::string const& message) {
    boost::asio::post(backoff_timer_.get_executor(),
                      [logger = logger_, message]() { logger(message); });
}

void CurlClient::report_error(Error error) {
    boost::asio::post(
        backoff_timer_.get_executor(),
        [errors = errors_, error = std::move(error)]() { errors(error); });
}
}  // namespace launchdarkly::sse

#endif  // LD_CURL_NETWORKING