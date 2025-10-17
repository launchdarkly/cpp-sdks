#ifdef LD_CURL_NETWORKING

#include "curl_client.hpp"

#include <boost/asio/post.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/url/url.hpp>

#include <sstream>

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

CurlClient::CurlClient(boost::asio::any_io_executor executor,
                       http::request<http::string_body> req,
                       std::string host,
                       std::string port,
                       std::optional<std::chrono::milliseconds> connect_timeout,
                       std::optional<std::chrono::milliseconds> read_timeout,
                       std::optional<std::chrono::milliseconds> write_timeout,
                       std::optional<std::chrono::milliseconds>
                       initial_reconnect_delay,
                       Builder::EventReceiver receiver,
                       Builder::LogCallback logger,
                       Builder::ErrorCallback errors,
                       bool skip_verify_peer,
                       std::optional<std::string> custom_ca_file,
                       bool use_https,
                       std::optional<std::string> proxy_url)
    :
    host_(std::move(host)),
    port_(std::move(port)),
    event_receiver_(std::move(receiver)),
    logger_(std::move(logger)),
    errors_(std::move(errors)),
    use_https_(use_https),
    backoff_timer_(executor),
    multi_manager_(CurlMultiManager::create(executor)),
    backoff_(initial_reconnect_delay.value_or(kDefaultInitialReconnectDelay),
             kDefaultMaxBackoffDelay) {
    request_context_ = std::make_shared<RequestContext>(
        build_url(req),
        std::move(req),
        connect_timeout,
        read_timeout,
        write_timeout,
        std::move(custom_ca_file),
        std::move(proxy_url),
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

void CurlClient::do_run() {
    if (request_context_->is_shutting_down()) {
        return;
    }

    auto ctx = request_context_;
    auto weak_self = weak_from_this();
    std::weak_ptr<RequestContext> weak_ctx = ctx;
    ctx->set_callbacks(Callbacks([weak_self, weak_ctx](const std::string& message) {
                                     if (auto ctx = weak_ctx.lock()) {
                                         if (auto self = weak_self.lock()) {
                                             boost::asio::post(
                                                 self->backoff_timer_.
                                                       get_executor(),
                                                 [self, message]() {
                                                     self->async_backoff(message);
                                                 });
                                         }
                                     }
                                 },
                                 [weak_self, weak_ctx](const Event& event) {
                                     if (auto ctx = weak_ctx.lock()) {
                                         if (auto self = weak_self.lock()) {
                                             boost::asio::post(
                                                 self->backoff_timer_.
                                                       get_executor(),
                                                 [self, event]() {
                                                     self->event_receiver_(event);
                                                 });
                                         }
                                     }
                                 },
                                 [weak_self, weak_ctx](const Error& error) {
                                     if (auto ctx = weak_ctx.lock()) {
                                         if (const auto self = weak_self.lock()) {
                                             // report_error does an asio post.
                                             self->report_error(error);
                                         }
                                     }
                                 },
                                 [weak_self, weak_ctx]() {
                                     if (auto ctx = weak_ctx.lock()) {
                                         if (const auto self = weak_self.lock()) {
                                             boost::asio::post(
                                                 self->backoff_timer_.
                                                       get_executor(),
                                                 [self]() {
                                                     self->backoff_.succeed();
                                                 });
                                         }
                                     }
                                 }, [weak_self, weak_ctx](const std::string& message) {
                                     if (auto ctx = weak_ctx.lock()) {
                                         if (const auto self = weak_self.lock()) {
                                             self->log_message(message);
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
    backoff_timer_.async_wait([weak_self](const boost::system::error_code& ec) {
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
    const http::request<http::string_body>& req) const {
    const std::string scheme = use_https_ ? "https" : "http";

    std::string url = scheme + "://" + host_;

    // Add port if it's not the default service name
    // port_ can be either a port number (like "8123") or service name (like "https"/"http")
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
#define CURL_SETOPT_CHECK(handle, option, parameter) \
        do { \
            CURLcode code = curl_easy_setopt(handle, option, parameter); \
            if (code != CURLE_OK) { \
                context.log_message("curl_easy_setopt failed for " #option ": " + \
                            std::string(curl_easy_strerror(code))); \
                return false; \
            } \
        } while(0)

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
        CURL_SETOPT_CHECK(curl, CURLOPT_POSTFIELDS,
                          context.req.body().c_str());
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

    // Add Last-Event-ID if we have one from previous connection
    if (context.last_event_id && !context.last_event_id->empty()) {
        std::string last_event_header = "Last-Event-ID: " + *context.last_event_id;
        headers = curl_slist_append(headers, last_event_header.c_str());
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
    // If proxy_url_ is std::nullopt, CURL will use environment variables (default behavior)

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
        const auto now = std::chrono::steady_clock::now();

        // If download amount has changed, update the last progress time
        if (dlnow != context->last_download_amount) {
            context->last_download_amount = dlnow;
            context->last_progress_time = now;
        } else {
            // No new data - check if we've exceeded the timeout
            const auto elapsed = std::chrono::duration_cast<
                std::chrono::milliseconds>(
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
                                             const curl_sockaddr* address) {
    auto* context = static_cast<RequestContext*>(clientp);

    // Create the socket
    curl_socket_t sockfd = socket(address->family, address->socktype,
                                  address->protocol);

    // Store it so we can close it during shutdown
    if (sockfd != CURL_SOCKET_BAD) {
        context->set_curl_socket(sockfd);
    }

    return sockfd;
}

// Callback for writing response data
//
// https://curl.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
size_t CurlClient::WriteCallback(const char* data,
                                 size_t size,
                                 size_t nmemb,
                                 void* userp) {
    size_t total_size = size * nmemb;
    auto* context = static_cast<RequestContext*>(userp);

    if (context->is_shutting_down()) {
        return 0; // Abort the transfer
    }

    // Set up the event receiver callback for the parser
    context->parser_body->on_event([context](Event event) {
        // Track last event ID for reconnection
        if (event.id()) {
            context->last_event_id = event.id();
        }
        context->receive(std::move(event));
    });

    const std::string_view data_view(data, total_size);
    context->parser_reader->put(data_view);

    return total_size;
}

// Callback for reading request headers
//
// https://curl.se/libcurl/c/CURLOPT_HEADERFUNCTION.html
size_t CurlClient::HeaderCallback(const char* buffer,
                                  size_t size,
                                  size_t nitems,
                                  void* userdata) {
    const size_t total_size = size * nitems;
    auto* client = static_cast<CurlClient*>(userdata);

    // Check for Content-Type header
    if (const std::string header(buffer, total_size);
        header.find("Content-Type:") == 0 ||
        header.find("content-type:") == 0) {
        if (header.find("text/event-stream") == std::string::npos) {
            client->log_message("warning: unexpected Content-Type: " + header);
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

    // Initialize parser for new connection (last_event_id is tracked separately)
    context->init_parser();

    CURL* curl = curl_easy_init();
    if (!curl) {
        if (context->is_shutting_down()) {
            return;
        }

        context->backoff("failed to initialize CURL");
        return;
    }

    curl_slist* headers = nullptr;
    if (!SetupCurlOptions(curl, &headers, *context)) {
        // setup_curl_options returned false, indicating an error (it already logged the error)
        curl_easy_cleanup(curl);

        if (context->is_shutting_down()) {
            return;
        }

        context->backoff("failed to set CURL options");
        return;
    }

    // Add handle to multi manager for async processing
    // Headers will be freed automatically by CurlMultiManager
    std::weak_ptr<RequestContext> weak_context = context;
    multi_manager->add_handle(curl, headers, [weak_context](CURL* easy, CURLcode res) {
        auto context = weak_context.lock();
        if (!context) {
            curl_easy_cleanup(easy);
            return;
        }

        // Get response code
        long response_code = 0;
        curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &response_code);

        // Handle HTTP status codes
        auto status = static_cast<http::status>(response_code);
        auto status_class = http::to_status_class(status);

        curl_easy_cleanup(easy);

        if (context->is_shutting_down()) {
            return;
        }

        if (status_class == http::status_class::redirection) {
            // The internal CURL handling of redirects failed.
            // This situation is likely the result of a missing redirect header
            // or empty header.
            context->error(errors::NotRedirectable{});
            return;
        }

        // Handle result
        if (res != CURLE_OK) {
            if (context->is_shutting_down()) {
                return;
            }

            // Check if the error was due to progress callback aborting (read timeout)
            if (res == CURLE_ABORTED_BY_CALLBACK && context->read_timeout) {
                context->error(errors::ReadTimeout{context->read_timeout});
                context->backoff("aborting read of response body (timeout)");
            } else {
                std::string error_msg = "CURL error: " + std::string(curl_easy_strerror(res));
                context->backoff(error_msg);
            }

            return;
        }

        if (status_class == http::status_class::successful) {
            if (status == http::status::no_content) {
                if (!context->is_shutting_down()) {
                    context->error(errors::UnrecoverableClientError{http::status::no_content});
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
                bool recoverable = (status == http::status::bad_request ||
                                    status == http::status::request_timeout ||
                                    status == http::status::too_many_requests);

                if (recoverable) {
                    std::stringstream ss;
                    ss << "HTTP status " << static_cast<int>(status);
                    context->backoff(ss.str());
                } else {
                    context->error(errors::UnrecoverableClientError{status});
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
    });
}

void CurlClient::async_shutdown(std::function<void()> completion) {
    boost::asio::post(backoff_timer_.get_executor(), [self = shared_from_this(),
                          completion = std::move(completion)]() {
                          self->do_shutdown(completion);
                      });
}

void CurlClient::do_shutdown(const std::function<void()>& completion) {
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
    boost::asio::post(backoff_timer_.get_executor(),
                      [errors = errors_, error = std::move(error)]() {
                          errors(error);
                      });
}
} // namespace launchdarkly::sse

#endif  // LD_CURL_NETWORKING