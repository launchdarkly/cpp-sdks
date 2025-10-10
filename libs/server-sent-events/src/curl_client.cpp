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

CurlClient::CurlClient(boost::asio::any_io_executor executor,
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
                       bool skip_verify_peer,
                       std::optional<std::string> custom_ca_file,
                       bool use_https)
    :
    host_(std::move(host)),
    port_(std::move(port)),
    req_(std::move(req)),
    connect_timeout_(connect_timeout),
    read_timeout_(read_timeout),
    write_timeout_(write_timeout),
    event_receiver_(std::move(receiver)),
    logger_(std::move(logger)),
    errors_(std::move(errors)),
    skip_verify_peer_(skip_verify_peer),
    custom_ca_file_(std::move(custom_ca_file)),
    use_https_(use_https),
    backoff_(initial_reconnect_delay.value_or(kDefaultInitialReconnectDelay),
             kDefaultMaxBackoffDelay),
    last_event_id_(std::nullopt),
    current_event_(std::nullopt),
    shutting_down_(false),
    curl_socket_(CURL_SOCKET_BAD),
    buffered_line_(std::nullopt),
    begin_CR_(false), last_download_amount_(0),
    backoff_timer_(std::move(executor)) {
}

CurlClient::~CurlClient() {
    shutting_down_ = true;
    backoff_timer_.cancel();

    // Close the socket to abort the CURL operation
    curl_socket_t sock = curl_socket_.load();
    if (sock != CURL_SOCKET_BAD) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
    }

    // Join the request thread if it exists and is joinable
    {
        std::lock_guard<std::mutex> lock(request_thread_mutex_);
        if (request_thread_ && request_thread_->joinable()) {
            // Release lock before joining to avoid holding mutex during join
            std::unique_ptr<std::thread> thread_to_join = std::move(request_thread_);
            request_thread_mutex_.unlock();
            thread_to_join->join();
            request_thread_mutex_.lock();
        }
        keepalive_.reset();
    }
}

void CurlClient::async_connect() {
    boost::asio::post(backoff_timer_.get_executor(),
                      [self = shared_from_this()]() { self->do_run(); });
}

void CurlClient::do_run() {
    if (shutting_down_) {
        return;
    }

    // Start request in a separate thread since CURL blocks
    {
        std::lock_guard<std::mutex> request_thread_guard(request_thread_mutex_);

        // Join any previous thread before starting a new one
        if (request_thread_ && request_thread_->joinable()) {
            request_thread_->join();
        }

        // Store a keepalive reference to ensure destructor doesn't run on request thread
        keepalive_ = shared_from_this();

        // Capture only raw 'this' pointer, not shared_ptr
        request_thread_ = std::make_unique<std::thread>(
            [this]() { this->perform_request(); });
    }
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
                                  boost::system::error_code ec) {
        self->on_backoff(ec);
    });
}

void CurlClient::on_backoff(boost::system::error_code ec) {
    if (ec == boost::asio::error::operation_aborted || shutting_down_) {
        return;
    }
    do_run();
}

std::string CurlClient::build_url() const {
    std::string scheme = use_https_ ? "https" : "http";

    std::string url = scheme + "://" + host_;

    // Add port if it's not the default service name
    // port_ can be either a port number (like "8123") or service name (like "https"/"http")
    if (port_ != "https" && port_ != "http") {
        url += ":" + port_;
    }

    url += std::string(req_.target());

    return url;
}

struct curl_slist* CurlClient::setup_curl_options(CURL* curl) {
    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, build_url().c_str());

    // Set HTTP method
    switch (req_.method()) {
        case http::verb::get:
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            break;
        case http::verb::post:
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            break;
        case http::verb::report:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "REPORT");
            break;
        default:
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            break;
    }

    // Set request body if present
    if (!req_.body().empty()) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_.body().c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, req_.body().size());
    }

    // Set headers
    struct curl_slist* headers = nullptr;
    for (auto const& field : req_) {
        std::string header = std::string(field.name_string()) + ": " +
                             std::string(field.value());
        headers = curl_slist_append(headers, header.c_str());
    }

    // Add Last-Event-ID if we have one
    if (last_event_id_ && !last_event_id_->empty()) {
        std::string last_event_header = "Last-Event-ID: " + *last_event_id_;
        headers = curl_slist_append(headers, last_event_header.c_str());
    }

    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    // Set timeouts with millisecond precision
    if (connect_timeout_) {
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, connect_timeout_->count());
    }

    // For read timeout, use progress callback
    if (read_timeout_) {
        effective_read_timeout_ = read_timeout_;
        last_progress_time_ = std::chrono::steady_clock::now();
        last_download_amount_ = 0;
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    }

    // Set TLS options
    if (skip_verify_peer_) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    } else {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

        if (custom_ca_file_) {
            curl_easy_setopt(curl, CURLOPT_CAINFO, custom_ca_file_->c_str());
        }
    }

    // Set callbacks
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
    curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, OpenSocketCallback);
    curl_easy_setopt(curl, CURLOPT_OPENSOCKETDATA, this);

    // Don't follow redirects automatically - we'll handle them ourselves
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);

    return headers;
}

int CurlClient::ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                                 curl_off_t ultotal, curl_off_t ulnow) {
    auto* client = static_cast<CurlClient*>(clientp);

    if (client->shutting_down_) {
        return 1;  // Abort the transfer
    }

    // Check if we've exceeded the read timeout
    if (client->effective_read_timeout_) {
        auto now = std::chrono::steady_clock::now();

        // If download amount has changed, update the last progress time
        if (dlnow != client->last_download_amount_) {
            client->last_download_amount_ = dlnow;
            client->last_progress_time_ = now;
        } else {
            // No new data - check if we've exceeded the timeout
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - client->last_progress_time_);

            if (elapsed > *client->effective_read_timeout_) {
                return 1;  // Abort the transfer
            }
        }
    }

    return 0;  // Continue
}

curl_socket_t CurlClient::OpenSocketCallback(void* clientp,
                                              curlsocktype purpose,
                                              struct curl_sockaddr* address) {
    auto* client = static_cast<CurlClient*>(clientp);

    // Create the socket
    curl_socket_t sockfd = socket(address->family, address->socktype, address->protocol);

    // Store it so we can close it during shutdown
    if (sockfd != CURL_SOCKET_BAD) {
        client->curl_socket_ = sockfd;
    }

    return sockfd;
}

size_t CurlClient::WriteCallback(char* data, size_t size, size_t nmemb,
                                  void* userp) {
    size_t total_size = size * nmemb;
    auto* client = static_cast<CurlClient*>(userp);

    if (client->shutting_down_) {
        return 0;  // Abort the transfer
    }

    // Parse SSE data
    std::string_view body(data, total_size);

    // Parse stream into lines
    size_t i = 0;
    while (i < body.size()) {
        // Find next line delimiter
        size_t delimiter_pos = body.find_first_of("\r\n", i);
        size_t append_size = (delimiter_pos == std::string::npos)
                                 ? (body.size() - i)
                                 : (delimiter_pos - i);

        // Append to buffered line
        if (client->buffered_line_.has_value()) {
            client->buffered_line_->append(body.substr(i, append_size));
        } else {
            client->buffered_line_ = std::string(body.substr(i, append_size));
        }

        i += append_size;

        if (i >= body.size()) {
            break;
        }

        // Handle line delimiters
        if (body[i] == '\r') {
            client->complete_lines_.push_back(*client->buffered_line_);
            client->buffered_line_.reset();
            client->begin_CR_ = true;
            i++;
        } else if (body[i] == '\n') {
            if (client->begin_CR_) {
                client->begin_CR_ = false;
            } else {
                client->complete_lines_.push_back(*client->buffered_line_);
                client->buffered_line_.reset();
            }
            i++;
        }
    }

    // Parse completed lines into events
    while (!client->complete_lines_.empty()) {
        std::string line = std::move(client->complete_lines_.front());
        client->complete_lines_.pop_front();

        if (line.empty()) {
            // Empty line indicates end of event
            if (client->current_event_) {
                // Trim trailing newline from data
                if (!client->current_event_->data.empty() &&
                    client->current_event_->data.back() == '\n') {
                    client->current_event_->data.pop_back();
                }

                // Update last_event_id_ only when dispatching a completed event
                if (client->current_event_->id) {
                    client->last_event_id_ = client->current_event_->id;
                }

                // Dispatch event on executor thread
                auto event_data = client->current_event_->data;
                auto event_type = client->current_event_->type.empty()
                                      ? "message"
                                      : client->current_event_->type;
                auto event_id = client->current_event_->id;

                boost::asio::post(client->backoff_timer_.get_executor(),
                                  [receiver = client->event_receiver_,
                                   type = std::move(event_type),
                                   data = std::move(event_data),
                                   id = std::move(event_id)]() {
                                      receiver(Event(type, data, id));
                                  });

                client->current_event_.reset();
            }
            continue;
        }

        // Parse field
        size_t colon_pos = line.find(':');
        if (colon_pos == 0) {
            // Comment line, dispatch it
            std::string comment = line.substr(1);
            boost::asio::post(client->backoff_timer_.get_executor(),
                              [receiver = client->event_receiver_,
                               comment = std::move(comment)]() {
                                  receiver(Event("comment", comment));
                              });
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
        if (!client->current_event_) {
            client->current_event_.emplace(detail::Event{});
            client->current_event_->id = client->last_event_id_;
        }

        // Handle field
        if (field_name == "event") {
            client->current_event_->type = field_value;
        } else if (field_name == "data") {
            client->current_event_->data += field_value;
            client->current_event_->data += '\n';
        } else if (field_name == "id") {
            if (field_value.find('\0') == std::string::npos) {
                client->current_event_->id = field_value;
            }
        }
        // retry field is ignored for now
    }

    return total_size;
}

size_t CurlClient::HeaderCallback(char* buffer, size_t size, size_t nitems,
                                   void* userdata) {
    size_t total_size = size * nitems;
    auto* client = static_cast<CurlClient*>(userdata);

    std::string header(buffer, total_size);

    // Check for Content-Type header
    if (header.find("Content-Type:") == 0 ||
        header.find("content-type:") == 0) {
        if (header.find("text/event-stream") == std::string::npos) {
            client->log_message("warning: unexpected Content-Type: " + header);
        }
    }

    return total_size;
}

bool CurlClient::handle_redirect(long response_code, CURL* curl) {
    // Check if this is a redirect status
    if (response_code != 301 && response_code != 307) {
        return false;
    }

    // Get the Location header
    char* location = nullptr;
    curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &location);

    if (!location || std::string(location).empty()) {
        // Invalid redirect, let FoxyClient behavior handle it
        return false;
    }

    // Parse the redirect URL
    auto location_url = boost::urls::parse_uri(location);
    if (!location_url) {
        report_error(errors::InvalidRedirectLocation{location});
        return true;
    }

    // Update host and target
    host_ = location_url->host();
    req_.set(http::field::host, host_);
    req_.target(location_url->encoded_target());

    if (location_url->has_port()) {
        port_ = location_url->port();
    } else {
        port_ = location_url->scheme();
    }

    // Signal that we should retry with the new location
    return true;
}

void CurlClient::perform_request() {
    // RAII guard to clear keepalive when function exits
    // Since we join the thread before destroying the object, we can safely clear keepalive here
    struct KeepaliveGuard {
        CurlClient* client;
        explicit KeepaliveGuard(CurlClient* c) : client(c) {}
        ~KeepaliveGuard() {
            std::lock_guard<std::mutex> lock(client->request_thread_mutex_);
            client->keepalive_.reset();
        }
    } guard(this);

    if (shutting_down_) {
        return;
    }

    // Clear parser state for new connection
    buffered_line_.reset();
    complete_lines_.clear();
    current_event_.reset();
    begin_CR_ = false;

    CURL* curl = curl_easy_init();
    if (!curl) {
        if (!shutting_down_) {
            boost::asio::post(backoff_timer_.get_executor(), [self = shared_from_this()]() {
                self->async_backoff("failed to initialize CURL");
            });
        }
        return;
    }

    struct curl_slist* headers = setup_curl_options(curl);

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

    // Handle redirects
    if (status_class == http::status_class::redirection) {
        bool should_redirect = handle_redirect(response_code, curl);
        curl_easy_cleanup(curl);

        if (should_redirect) {
            // Retry with new location
            if (!shutting_down_) {
                perform_request();
            }
            return;
        }
        // Invalid redirect - report error
        if (!shutting_down_) {
            boost::asio::post(backoff_timer_.get_executor(), [self = shared_from_this()]() {
                self->report_error(errors::NotRedirectable{});
            });
        }
        return;
    }

    curl_easy_cleanup(curl);

    if (shutting_down_) {
        return;
    }

    // Handle result
    if (res != CURLE_OK) {
        if (!shutting_down_) {
            // Check if the error was due to progress callback aborting (read timeout)
            if (res == CURLE_ABORTED_BY_CALLBACK && effective_read_timeout_) {
                boost::asio::post(backoff_timer_.get_executor(), [self = shared_from_this()]() {
                    self->report_error(errors::ReadTimeout{self->read_timeout_});
                    self->async_backoff("aborting read of response body (timeout)");
                });
            } else {
                std::string error_msg = "CURL error: " + std::string(curl_easy_strerror(res));
                boost::asio::post(backoff_timer_.get_executor(), [self = shared_from_this(),
                                              error_msg = std::move(error_msg)]() {
                    self->async_backoff(error_msg);
                });
            }
        }
        return;
    }

    if (status_class == http::status_class::successful) {
        if (status == http::status::no_content) {
            if (!shutting_down_) {
                boost::asio::post(backoff_timer_.get_executor(), [self = shared_from_this()]() {
                    self->report_error(errors::UnrecoverableClientError{http::status::no_content});
                });
            }
            return;
        }
        if (!shutting_down_) {
            log_message("connected");
        }
        backoff_.succeed();
        // Connection ended normally, reconnect
        if (!shutting_down_) {
            boost::asio::post(backoff_timer_.get_executor(),
                              [self = shared_from_this()]() {
                                  self->async_backoff("connection closed normally");
                              });
        }
        return;
    }

    if (status_class == http::status_class::client_error) {
        if (!shutting_down_) {
            bool recoverable = (status == http::status::bad_request ||
                                status == http::status::request_timeout ||
                                status == http::status::too_many_requests);

            if (recoverable) {
                std::stringstream ss;
                ss << "HTTP status " << static_cast<int>(status);
                boost::asio::post(backoff_timer_.get_executor(), [self = shared_from_this(),
                                              reason = ss.str()]() {
                    self->async_backoff(reason);
                });
            } else {
                boost::asio::post(backoff_timer_.get_executor(), [self = shared_from_this(), status]() {
                    self->report_error(errors::UnrecoverableClientError{status});
                });
            }
        }
        return;
    }

    // Server error or other - backoff and retry
    if (!shutting_down_) {
        std::stringstream ss;
        ss << "HTTP status " << static_cast<int>(status);
        boost::asio::post(backoff_timer_.get_executor(),
                          [self = shared_from_this(), reason = ss.str()]() {
                              self->async_backoff(reason);
                          });
    }

    // Keepalive will be cleared by guard's destructor when function exits
}

void CurlClient::async_shutdown(std::function<void()> completion) {
    boost::asio::post(backoff_timer_.get_executor(), [self = shared_from_this(),
                                  completion = std::move(completion)]() {
        self->do_shutdown(std::move(completion));
    });
}

void CurlClient::do_shutdown(std::function<void()> completion) {
    shutting_down_ = true;
    backoff_timer_.cancel();

    // Close the socket to abort the CURL operation
    curl_socket_t sock = curl_socket_.load();
    if (sock != CURL_SOCKET_BAD) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
    }

    // Join the request thread if it exists and is joinable
    {
        std::lock_guard<std::mutex> lock(request_thread_mutex_);
        if (request_thread_ && request_thread_->joinable()) {
            // Release lock before joining to avoid holding mutex during join
            std::unique_ptr<std::thread> thread_to_join = std::move(request_thread_);
            request_thread_mutex_.unlock();
            thread_to_join->join();
            request_thread_mutex_.lock();
        }
        keepalive_.reset();
    }

    if (completion) {
        completion();
    }
}

void CurlClient::log_message(std::string const& message) {
    boost::asio::post(backoff_timer_.get_executor(),
                      [logger = logger_, message]() { logger(message); });
}

void CurlClient::report_error(Error error) {
    boost::asio::post(backoff_timer_.get_executor(), [errors = errors_, error = std::move(error)]() {
        errors(error);
    });
}

}  // namespace launchdarkly::sse
