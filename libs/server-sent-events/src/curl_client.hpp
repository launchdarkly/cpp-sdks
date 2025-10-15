#pragma once

#ifdef LD_CURL_NETWORKING

#include <launchdarkly/sse/client.hpp>
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
#include <boost/asio/post.hpp>

namespace launchdarkly::sse {
namespace http = boost::beast::http;
namespace net = boost::asio;

struct RequestContext {

    // SSE parser state
    std::optional<std::string> buffered_line_;
    std::deque<std::string> complete_lines_;
    bool begin_CR_;

    // Progress tracking for read timeout
    std::chrono::steady_clock::time_point last_progress_time_;
    curl_off_t last_download_amount_;
    std::optional<std::chrono::milliseconds> effective_read_timeout_;

    // Only items used by both the curl thread and the executor/main
    // thread need to be mutex protected.
    std::mutex mutex_;
    std::atomic<bool> shutting_down_;
    std::atomic<curl_socket_t> curl_socket_;
    std::function<void(std::string)> doBackoff_;
    std::function<void(Event)> onReceive_;
    std::function<void(Error)> onError_;
    std::function<void()> resetBackoff_;
    // End mutex protected items.

    std::optional<std::string> last_event_id_;
    std::optional<detail::Event> current_event_;

    boost::asio::any_io_executor executor_;
    http::request<http::string_body> req_;
    std::string url_;

    std::optional<std::chrono::milliseconds> connect_timeout_;
    std::optional<std::chrono::milliseconds> read_timeout_;
    std::optional<std::chrono::milliseconds> write_timeout_;
    std::optional<std::string> custom_ca_file_;
    std::optional<std::string> proxy_url_;

    bool skip_verify_peer_;

    void backoff(const std::string& message) {
        std::lock_guard lock(mutex_);
        if (shutting_down_) {
            return;
        }
        doBackoff_(message);
    }

    void error(const Error& error) {
        std::lock_guard lock(mutex_);
        if (shutting_down_) {
            return;
        }
        onError_(error);
    }

    void receive(const Event& event) {
        std::lock_guard lock(mutex_);
        if (shutting_down_) {
            return;
        }
        onReceive_(event);
    }

    RequestContext(std::string url,
                   http::request<http::string_body> req,
                   std::optional<std::chrono::milliseconds> connect_timeout,
                   std::optional<std::chrono::milliseconds> read_timeout,
                   std::optional<std::chrono::milliseconds> write_timeout,
                   std::optional<std::string> custom_ca_file,
                   std::optional<std::string> proxy_url,
                   bool skip_verify_peer,
                   std::function<void(std::string)> doBackoff,
                   std::function<void(Event)> onReceive,
                   std::function<void(Error)> onError,
                   std::function<void()> resetBackoff
        ) : curl_socket_(CURL_SOCKET_BAD),
            buffered_line_(std::nullopt),
            begin_CR_(false),
            last_download_amount_(0),
            shutting_down_(false),
            last_event_id_(std::nullopt), current_event_(std::nullopt),
            req_(std::move(req)),
            url_(std::move(url)),
            connect_timeout_(connect_timeout),
            read_timeout_(read_timeout),
            write_timeout_(write_timeout),
            custom_ca_file_(std::move(custom_ca_file)),
            proxy_url_(std::move(proxy_url)),
            skip_verify_peer_(skip_verify_peer),
            doBackoff_(std::move(doBackoff)),
            onReceive_(std::move(onReceive)),
            onError_(std::move(onError)),
            resetBackoff_(std::move(resetBackoff)) {
    }
};

class CurlClient : public Client,
                   public std::enable_shared_from_this<CurlClient> {
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
    void do_run() const;
    void do_shutdown(const std::function<void()>& completion);
    void async_backoff(std::string const& reason);
    void on_backoff(const boost::system::error_code& ec) const;
    static void PerformRequest(
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

    std::string build_url(http::request<http::string_body> req) const;
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

    Backoff backoff_;
};
} // namespace launchdarkly::sse

#endif  // LD_CURL_NETWORKING