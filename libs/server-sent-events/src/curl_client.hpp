#pragma once

#ifdef LD_CURL_NETWORKING

#include <launchdarkly/sse/client.hpp>
#include <launchdarkly/network/curl_multi_manager.hpp>
#include "backoff.hpp"
#include "parser.hpp"

#include <curl/curl.h>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/http/string_body.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace launchdarkly::sse {
namespace http = beast::http;
namespace net = boost::asio;

using launchdarkly::network::CurlMultiManager;

/**
 * The lifecycle of the CurlClient is managed in an RAII manner. This introduces
 * some complexity with interaction with CURL, which requires a thread to be
 * driven. The desutruction of the CurlClient will signal that the CURL thread
 * should stop. Though depending on the operation it may linger for some
 * time.
 *
 * The CurlClient itself is not accessible from the CURL thread and instead
 * all their shared state is in a RequestContext. The lifetime of this context
 * can exceed that of the CurlClient while CURL shuts down. This approach
 * prevents the lifetime of the CurlClient being attached to that of the
 * curl thread.
 */
class CurlClient final : public Client,
                         public std::enable_shared_from_this<CurlClient> {
    struct Callbacks {
        std::function<void(std::string)> do_backoff;
        std::function<void(Event)> on_receive;
        std::function<void(Error)> on_error;
        std::function<void()> reset_backoff;
        std::function<void(std::string)> log_message;

        Callbacks(
            std::function<void(std::string)> do_backoff,
            std::function<void(Event)> on_receive,
            std::function<void(Error)> on_error,
            std::function<void()> reset_backoff,
            std::function<void(std::string)> log_message
            ) :
            do_backoff(std::move(do_backoff)),
            on_receive(std::move(on_receive)),
            on_error(std::move(on_error)),
            reset_backoff(std::move(reset_backoff)),
            log_message(std::move(log_message)) {
        }
    };

    class RequestContext {
        // Only items used by both the curl thread and the executor/main
        // thread need to be mutex protected.
        std::mutex mutex_;
        std::atomic<bool> shutting_down_;
        std::atomic<curl_socket_t> curl_socket_;
        // End mutex protected items.
        std::optional<Callbacks> callbacks_;

    public:
        // SSE parser state
        std::optional<std::string> buffered_line;
        std::deque<std::string> complete_lines;
        bool begin_CR;

        // Progress tracking for read timeout
        std::chrono::steady_clock::time_point last_progress_time;
        curl_off_t last_download_amount;


        std::optional<std::string> last_event_id;
        std::optional<detail::Event> current_event;

        const http::request<http::string_body> req;
        const std::string url;
        const std::optional<std::chrono::milliseconds> connect_timeout;
        const std::optional<std::chrono::milliseconds> read_timeout;
        const std::optional<std::chrono::milliseconds> write_timeout;
        const std::optional<std::string> custom_ca_file;
        const std::optional<std::string> proxy_url;
        const bool skip_verify_peer;

        void backoff(const std::string& message) {
            std::lock_guard lock(mutex_);
            if (shutting_down_) {
                return;
            }
            if (callbacks_) {
                callbacks_->do_backoff(message);
            }
        }

        void error(const Error& error) {
            std::lock_guard lock(mutex_);
            if (shutting_down_) {
                return;
            }
            if (callbacks_) {
                callbacks_->on_error(error);
            }
        }

        void receive(const Event& event) {
            std::lock_guard lock(mutex_);
            if (shutting_down_) {
                return;
            }
            if (callbacks_) {
                callbacks_->on_receive(event);
            }
        }

        void reset_backoff() {
            std::lock_guard lock(mutex_);
            if (shutting_down_) {
                return;
            }
            if (callbacks_) {
                callbacks_->reset_backoff();
            }
        }

        void log_message(const std::string& message) {
            std::lock_guard lock(mutex_);
            if (shutting_down_) {
                return;
            }
            if (callbacks_) {
                callbacks_->log_message(message);
            }
        }

        void set_callbacks(Callbacks callbacks) {
            std::lock_guard lock(mutex_);
            callbacks_ = std::move(callbacks);
        }

        bool is_shutting_down() {
            return shutting_down_;
        }

        void set_curl_socket(curl_socket_t curl_socket) {
            std::lock_guard lock(mutex_);
            curl_socket_ = curl_socket;
        }

        void shutdown() {
            std::lock_guard lock(mutex_);
            shutting_down_ = true;
            if (curl_socket_ != CURL_SOCKET_BAD) {
#ifdef _WIN32
                closesocket(curl_socket_);
#else
                close(curl_socket_);
#endif
            }
        }


        RequestContext(std::string url,
                       http::request<http::string_body> req,
                       std::optional<std::chrono::milliseconds> connect_timeout,
                       std::optional<std::chrono::milliseconds> read_timeout,
                       std::optional<std::chrono::milliseconds> write_timeout,
                       std::optional<std::string> custom_ca_file,
                       std::optional<std::string> proxy_url,
                       bool skip_verify_peer
            ) : shutting_down_(false),
                curl_socket_(CURL_SOCKET_BAD),
                buffered_line(std::nullopt),
                begin_CR(false),
                last_download_amount(0),
                last_event_id(std::nullopt),
                current_event(std::nullopt),
                req(std::move(req)),
                url(std::move(url)),
                connect_timeout(connect_timeout),
                read_timeout(read_timeout),
                write_timeout(write_timeout),
                custom_ca_file(std::move(custom_ca_file)),
                proxy_url(std::move(proxy_url)),
                skip_verify_peer(skip_verify_peer) {
        }
    };

public:
    CurlClient(boost::asio::any_io_executor executor,
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
               bool use_https,
               std::optional<std::string> proxy_url);

    ~CurlClient() override;

    void async_connect() override;
    void async_shutdown(std::function<void()> completion) override;

private:
    void do_run();
    void do_shutdown(const std::function<void()>& completion);
    void async_backoff(std::string const& reason);
    void on_backoff(boost::system::error_code const& ec);
    static void PerformRequestWithMulti(
        std::shared_ptr<CurlMultiManager> multi_manager,
        std::shared_ptr<RequestContext> context);

    static size_t WriteCallback(const char* data,
                                size_t size,
                                size_t nmemb,
                                void* userp);
    static size_t HeaderCallback(const char* buffer,
                                 size_t size,
                                 size_t nitems,
                                 void* userdata);
    static curl_socket_t OpenSocketCallback(void* clientp,
                                            curlsocktype purpose,
                                            const struct curl_sockaddr*
                                            address);

    void log_message(std::string const& message);
    void report_error(Error error);

    std::string build_url(const http::request<http::string_body>& req) const;
    static bool SetupCurlOptions(CURL* curl,
                                 curl_slist** headers,
                                 RequestContext& context);

    static int ProgressCallback(void* clientp,
                                curl_off_t dltotal,
                                curl_off_t dlnow,
                                curl_off_t ultotal,
                                curl_off_t ulnow);

    std::shared_ptr<RequestContext> request_context_;

    std::string host_;
    std::string port_;

    Builder::EventReceiver event_receiver_;
    Builder::LogCallback logger_;
    Builder::ErrorCallback errors_;

    bool use_https_;
    boost::asio::steady_timer backoff_timer_;
    std::shared_ptr<CurlMultiManager> multi_manager_;

    Backoff backoff_;
};
} // namespace launchdarkly::sse

#endif  // LD_CURL_NETWORKING