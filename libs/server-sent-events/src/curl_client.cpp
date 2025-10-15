#ifdef LD_CURL_NETWORKING

#include "curl_client.hpp"

#include <boost/asio/post.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/url/parse.hpp>
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
    backoff_timer_(std::move(executor)),
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
        skip_verify_peer,
        [this](const std::string& message) {
            boost::asio::post(backoff_timer_.get_executor(), [this, message]() {
                async_backoff(message);
            });
        },
        [this](const Event& event) {
            boost::asio::post(backoff_timer_.get_executor(), [this, event]() {
                event_receiver_(event);
            });
        },
        [this](const Error& error) {
            boost::asio::post(backoff_timer_.get_executor(), [this, error]() {
                report_error(error);
            });
        },
        [this]() {
            boost::asio::post(backoff_timer_.get_executor(), [this]() {
                backoff_.succeed();
            });
        });
}

CurlClient::~CurlClient() {
    {
        // When shutting down we want to retain the lock while we set shutdown.
        // This will ensure this blocks until any outstanding use of the "this"
        // pointer is complete.
        std::lock_guard lock(request_context_->mutex_);
        request_context_->shutting_down_ = true;
        // Close the socket to abort the CURL operation
        curl_socket_t sock = request_context_->curl_socket_.load();
        if (sock != CURL_SOCKET_BAD) {
#ifdef _WIN32
            closesocket(sock);
#else
            close(sock);
#endif
        }
    }
    backoff_timer_.cancel();
}

void CurlClient::async_connect() {
    boost::asio::post(backoff_timer_.get_executor(),
                      [self = shared_from_this()]() { self->do_run(); });
}

void CurlClient::do_run() const {
    if (request_context_->shutting_down_) {
        return;
    }

    auto ctx = request_context_;
    // Start request in a separate thread since CURL blocks
    // Capture only raw 'this' pointer, not shared_ptr
    std::thread t(
        [ctx]() { PerformRequest(ctx); });

    t.detach();
}

void CurlClient::async_backoff(std::string const& reason) {
    backoff_.fail();

    std::stringstream msg;
    msg << "backing off in ("
        << std::chrono::duration_cast<std::chrono::seconds>(backoff_.delay())
        .count()
        << ") seconds due to " << reason;

    log_message(msg.str());

    backoff_timer_.expires_after(backoff_.delay());
    backoff_timer_.async_wait([self = shared_from_this()](
        const boost::system::error_code& ec) {
            self->on_backoff(ec);
        });
}

void CurlClient::on_backoff(const boost::system::error_code& ec) const {
    {
        if (ec == boost::asio::error::operation_aborted || request_context_->
            shutting_down_) {
            return;
        }
    }
    do_run();
}

std::string CurlClient::build_url(http::request<http::string_body> req) const {
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
                /*log_message("curl_easy_setopt failed for " #option ": " +*/ \
                            /*std::string(curl_easy_strerror(code))); */\
                return false; \
            } \
        } while(0)

    // Set URL
    CURL_SETOPT_CHECK(curl, CURLOPT_URL, context.url_.c_str());

    // Set HTTP method
    switch (context.req_.method()) {
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
    if (!context.req_.body().empty()) {
        CURL_SETOPT_CHECK(curl, CURLOPT_POSTFIELDS,
                          context.req_.body().c_str());
        CURL_SETOPT_CHECK(curl, CURLOPT_POSTFIELDSIZE,
                          context.req_.body().size());
    }

    // Set headers
    struct curl_slist* headers = nullptr;
    for (auto const& field : context.req_) {
        std::string header = std::string(field.name_string()) + ": " +
                             std::string(field.value());
        headers = curl_slist_append(headers, header.c_str());
    }

    // Add Last-Event-ID if we have one
    if (context.last_event_id_ && !context.last_event_id_->empty()) {
        std::string last_event_header =
            "Last-Event-ID: " + *context.last_event_id_;
        headers = curl_slist_append(headers, last_event_header.c_str());
    }

    if (headers) {
        CURL_SETOPT_CHECK(curl, CURLOPT_HTTPHEADER, headers);
    }

    // Set timeouts with millisecond precision
    if (context.connect_timeout_) {
        CURL_SETOPT_CHECK(curl, CURLOPT_CONNECTTIMEOUT_MS,
                          context.connect_timeout_->count());
    }

    // For read timeout, use progress callback
    if (context.read_timeout_) {
        context.effective_read_timeout_ = context.read_timeout_;
        context.last_progress_time_ = std::chrono::steady_clock::now();
        context.last_download_amount_ = 0;
        CURL_SETOPT_CHECK(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
        CURL_SETOPT_CHECK(curl, CURLOPT_XFERINFODATA, &context);
        CURL_SETOPT_CHECK(curl, CURLOPT_NOPROGRESS, 0L);
    }

    // Set TLS options
    if (context.skip_verify_peer_) {
        CURL_SETOPT_CHECK(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        CURL_SETOPT_CHECK(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    } else {
        CURL_SETOPT_CHECK(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        CURL_SETOPT_CHECK(curl, CURLOPT_SSL_VERIFYHOST, 2L);

        if (context.custom_ca_file_) {
            CURL_SETOPT_CHECK(curl, CURLOPT_CAINFO,
                              context.custom_ca_file_->c_str());
        }
    }

    // Set proxy if configured
    // When proxy_url_ is set, it takes precedence over environment variables.
    // Empty string explicitly disables proxy (overrides environment variables).
    if (context.proxy_url_) {
        CURL_SETOPT_CHECK(curl, CURLOPT_PROXY, context.proxy_url_->c_str());
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

    if (context->shutting_down_) {
        return kCurlTransferAbort;
    }

    // Check if we've exceeded the read timeout
    if (context->effective_read_timeout_) {
        const auto now = std::chrono::steady_clock::now();

        // If download amount has changed, update the last progress time
        if (dlnow != context->last_download_amount_) {
            context->last_download_amount_ = dlnow;
            context->last_progress_time_ = now;
        } else {
            // No new data - check if we've exceeded the timeout
            auto elapsed = std::chrono::duration_cast<
                std::chrono::milliseconds>(
                now - context->last_progress_time_);

            if (elapsed > *context->effective_read_timeout_) {
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
        std::lock_guard lock(context->mutex_);
        context->curl_socket_ = sockfd;
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

    if (context->shutting_down_) {
        return 0; // Abort the transfer
    }

    // Parse SSE data
    std::string_view body(data, total_size);

    // Parse stream into lines
    size_t i = 0;
    while (i < body.size()) {
        // Find next line delimiter
        const size_t delimiter_pos = body.find_first_of("\r\n", i);
        const size_t append_size = (delimiter_pos == std::string::npos)
                                       ? (body.size() - i)
                                       : (delimiter_pos - i);

        // Append to buffered line
        if (context->buffered_line_.has_value()) {
            context->buffered_line_->append(body.substr(i, append_size));
        } else {
            context->buffered_line_ = std::string(body.substr(i, append_size));
        }

        i += append_size;

        if (i >= body.size()) {
            break;
        }

        // Handle line delimiters
        if (body[i] == '\r') {
            context->complete_lines_.push_back(*context->buffered_line_);
            context->buffered_line_.reset();
            context->begin_CR_ = true;
            i++;
        } else if (body[i] == '\n') {
            if (context->begin_CR_) {
                context->begin_CR_ = false;
            } else {
                context->complete_lines_.push_back(*context->buffered_line_);
                context->buffered_line_.reset();
            }
            i++;
        }
    }

    // Parse completed lines into events
    while (!context->complete_lines_.empty()) {
        std::string line = std::move(context->complete_lines_.front());
        context->complete_lines_.pop_front();

        if (line.empty()) {
            // Empty line indicates end of event
            if (context->current_event_) {
                // Trim trailing newline from data
                if (!context->current_event_->data.empty() &&
                    context->current_event_->data.back() == '\n') {
                    context->current_event_->data.pop_back();
                }

                // Update last_event_id_ only when dispatching a completed event
                if (context->current_event_->id) {
                    context->last_event_id_ = context->current_event_->id;
                }

                // Dispatch event on executor thread
                auto event_data = context->current_event_->data;
                auto event_type = context->current_event_->type.empty()
                                      ? "message"
                                      : context->current_event_->type;
                auto event_id = context->current_event_->id;
                context->receive(Event(
                    std::move(event_type),
                    std::move(event_data),
                    std::move(event_id)));

                context->current_event_.reset();
            }
            continue;
        }

        // Parse field
        const size_t colon_pos = line.find(':');
        if (colon_pos == 0) {
            // Comment line, dispatch it
            std::string comment = line.substr(1);

            context->receive(Event("comment", comment));
            continue;
        }

        std::string field_name;
        std::string field_value;

        if (colon_pos == std::string::npos) {
            field_name = line;
            field_value = "";
        } else {
            field_name = line.substr(0, colon_pos);
            field_value = line.substr(colon_pos + 1);

            // Remove leading space from value if present
            if (!field_value.empty() && field_value[0] == ' ') {
                field_value = field_value.substr(1);
            }
        }

        // Initialize event if needed
        if (!context->current_event_) {
            context->current_event_.emplace(detail::Event{});
            context->current_event_->id = context->last_event_id_;
        }

        // Handle field
        if (field_name == "event") {
            context->current_event_->type = field_value;
        } else if (field_name == "data") {
            context->current_event_->data += field_value;
            context->current_event_->data += '\n';
        } else if (field_name == "id") {
            if (field_value.find('\0') == std::string::npos) {
                context->current_event_->id = field_value;
            }
        }
        // retry field is ignored for now
    }

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

void CurlClient::PerformRequest(std::shared_ptr<RequestContext> context) {
    if (context->shutting_down_) {
        return;
    }

    // Clear parser state for new connection
    context->buffered_line_.reset();
    context->complete_lines_.clear();
    context->current_event_.reset();
    context->begin_CR_ = false;

    CURL* curl = curl_easy_init();
    if (!curl) {
        if (context->shutting_down_) {
            return;
        }

        context->backoff("failed to initialize CURL");
        return;
    }

    curl_slist* headers = nullptr;
    if (!SetupCurlOptions(curl, &headers, *context)) {
        // setup_curl_options returned false, indicating an error (it already logged the error)
        curl_easy_cleanup(curl);

        if (context->shutting_down_) {
            return;
        }

        context->backoff("failed to set CURL options");
        return;
    }

    // Perform the request
    CURLcode res = curl_easy_perform(curl);

    // Get response code
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    // Free headers before cleanup
    if (headers) {
        curl_slist_free_all(headers);
    }

    // Handle HTTP status codes
    auto status = static_cast<http::status>(response_code);
    auto status_class = http::to_status_class(status);

    curl_easy_cleanup(curl);

    if (context->shutting_down_) {
        return;
    }

    if (status_class == http::status_class::redirection) {
        // The internal CURL handling of redirects failed.
        // This situation is likely the result of a missing redirect header
        // or empty header.
        context->error(errors::NotRedirectable{});
    }

    // Handle result
    if (res != CURLE_OK) {
        if (context->shutting_down_) {
            return;
        }

        // Check if the error was due to progress callback aborting (read timeout)
        if (res == CURLE_ABORTED_BY_CALLBACK && context->
            effective_read_timeout_) {
            context->error(errors::ReadTimeout{
                context->read_timeout_
            });
            context->backoff("aborting read of response body (timeout)");
        } else {
            std::string error_msg = "CURL error: " + std::string(
                                        curl_easy_strerror(res));
            context->backoff(error_msg);
        }

        return;
    }

    if (status_class == http::status_class::successful) {
        if (status == http::status::no_content) {
            if (!context->shutting_down_) {
                context->error(errors::UnrecoverableClientError{
                    http::status::no_content});
            }
            return;
        }
        if (!context->shutting_down_) {
            // log_message("connected");
        }
        context->resetBackoff_();
        // Connection ended normally, reconnect
        if (!context->shutting_down_) {
            context->backoff("connection closed normally");
        }
        return;
    }

    if (status_class == http::status_class::client_error) {
        if (!context->shutting_down_) {
            bool recoverable = (status == http::status::bad_request ||
                                status == http::status::request_timeout ||
                                status == http::status::too_many_requests);

            if (recoverable) {
                std::stringstream ss;
                ss << "HTTP status " << static_cast<int>(status);
                context->backoff(ss.str());
            } else {
                context->error(errors::UnrecoverableClientError{
                    status});
            }
        }
        return;
    }

    {
        // Server error or other - backoff and retry
        if (!context->shutting_down_) {
            std::stringstream ss;
            ss << "HTTP status " << static_cast<int>(status);
            context->backoff(ss.str());
        }
    }

    // Keepalive will be cleared by guard's destructor when function exits
}

void CurlClient::async_shutdown(std::function<void()> completion) {
    boost::asio::post(backoff_timer_.get_executor(), [self = shared_from_this(),
                          completion = std::move(completion)]() {
                          self->do_shutdown(completion);
                      });
}

void CurlClient::do_shutdown(const std::function<void()>& completion) {
    request_context_->shutting_down_ = true;
    {
        std::lock_guard lock(request_context_->mutex_);
        // Close the socket to abort the CURL operation
        curl_socket_t sock = request_context_->curl_socket_.load();
        if (sock != CURL_SOCKET_BAD) {
#ifdef _WIN32
            closesocket(sock);
#else
            close(sock);
#endif
        }
    }
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