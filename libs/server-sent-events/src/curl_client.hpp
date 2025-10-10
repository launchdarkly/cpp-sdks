#pragma once

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

namespace launchdarkly::sse {

namespace http = boost::beast::http;
namespace net = boost::asio;

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
               bool use_https);

    ~CurlClient() override;

    void async_connect() override;
    void async_shutdown(std::function<void()> completion) override;

   private:
    void do_run();
    void do_shutdown(std::function<void()> completion);
    void async_backoff(std::string const& reason);
    void on_backoff(boost::system::error_code ec);
    void perform_request();

    static size_t WriteCallback(char* data, size_t size, size_t nmemb, void* userp);
    static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata);
    static curl_socket_t OpenSocketCallback(void* clientp, curlsocktype purpose, struct curl_sockaddr* address);

    void log_message(std::string const& message);
    void report_error(Error error);

    std::string build_url() const;
    struct curl_slist* setup_curl_options(CURL* curl);
    bool handle_redirect(long response_code, CURL* curl);

    static int ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                                curl_off_t ultotal, curl_off_t ulnow);

    std::string host_;
    std::string port_;
    http::request<http::string_body> req_;

    std::optional<std::chrono::milliseconds> connect_timeout_;
    std::optional<std::chrono::milliseconds> read_timeout_;
    std::optional<std::chrono::milliseconds> write_timeout_;

    Builder::EventReceiver event_receiver_;
    Builder::LogCallback logger_;
    Builder::ErrorCallback errors_;

    bool skip_verify_peer_;
    std::optional<std::string> custom_ca_file_;
    bool use_https_;

    Backoff backoff_;

    std::optional<std::string> last_event_id_;
    std::optional<detail::Event> current_event_;

    std::atomic<bool> shutting_down_;
    std::atomic<curl_socket_t> curl_socket_;

    std::unique_ptr<std::thread> request_thread_;
    std::mutex request_thread_mutex_;

    // Keepalive reference to prevent destructor from running on request thread
    std::shared_ptr<CurlClient> keepalive_;

    // SSE parser state
    std::optional<std::string> buffered_line_;
    std::deque<std::string> complete_lines_;
    bool begin_CR_;

    // Progress tracking for read timeout
    std::chrono::steady_clock::time_point last_progress_time_;
    curl_off_t last_download_amount_;
    std::optional<std::chrono::milliseconds> effective_read_timeout_;
    boost::asio::steady_timer backoff_timer_;
};

}  // namespace launchdarkly::sse
