#pragma once

#ifdef LD_CURL_NETWORKING

#include <curl/curl.h>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>

#include <functional>
#include <map>
#include <memory>
#include <mutex>

namespace launchdarkly::network {

// Use tcp::socket for cross-platform socket operations
using SocketHandle = boost::asio::ip::tcp::socket;

/**
 * Manages CURL multi interface integrated with ASIO event loop.
 *
 * This class provides non-blocking HTTP operations by integrating CURL's
 * multi interface with Boost.ASIO. Instead of blocking threads, CURL notifies
 * us via callbacks when sockets need attention, and we use ASIO to monitor
 * those sockets asynchronously.
 *
 * Key features:
 * - Non-blocking I/O using curl_multi_socket_action
 * - Cross-platform socket monitoring via ASIO tcp::socket
 * - Timer integration with ASIO steady_timer
 * - Thread-safe operation on ASIO executor
 */
class CurlMultiManager : public std::enable_shared_from_this<CurlMultiManager> {
public:
    /**
     * Callback invoked when an easy handle completes (success or error).
     * Parameters: CURL* easy handle, CURLcode result
     */
    using CompletionCallback = std::function<void(CURL*, CURLcode)>;

    /**
     * Create a CurlMultiManager on the given executor.
     * @param executor The ASIO executor to run operations on
     */
    static std::shared_ptr<CurlMultiManager> create(
        boost::asio::any_io_executor executor);

    ~CurlMultiManager();

    // Non-copyable and non-movable
    CurlMultiManager(const CurlMultiManager&) = delete;
    CurlMultiManager& operator=(const CurlMultiManager&) = delete;
    CurlMultiManager(CurlMultiManager&&) = delete;
    CurlMultiManager& operator=(CurlMultiManager&&) = delete;

    /**
     * Add an easy handle to be managed.
     * @param easy The CURL easy handle (must be configured)
     * @param headers The curl_slist headers (will be freed automatically)
     * @param callback Called when the transfer completes
     */
    void add_handle(std::shared_ptr<CURL> easy,
                    curl_slist* headers,
                    CompletionCallback callback);

    /**
     * Remove an easy handle from management.
     * @param easy The CURL easy handle to remove
     */
    void remove_handle(CURL* easy);

private:
    explicit CurlMultiManager(boost::asio::any_io_executor executor);

    // Called by CURL when socket state changes
    static int socket_callback(CURL* easy,
                               curl_socket_t s,
                               int what,
                               void* userp,
                               void* socketp);

    // Called by CURL when timer should be set
    static int timer_callback(CURLM* multi, long timeout_ms, void* userp);

    // Handle socket events
    void handle_socket_action(curl_socket_t s, int event_bitmask);

    // Handle timer expiry
    void handle_timeout();

    // Check for completed transfers
    void check_multi_info();

    // Per-socket data
    struct SocketInfo {
        curl_socket_t sockfd;
        std::shared_ptr<SocketHandle> handle;
        int action{0}; // CURL_POLL_IN, CURL_POLL_OUT, etc.
        // Keep handlers alive - we own them and they only capture weak_ptr to avoid circular refs
        std::shared_ptr<std::function<void()>> read_handler;
        std::shared_ptr<std::function<void()>> write_handler;
    };

    void start_socket_monitor(SocketInfo* socket_info, int action);

    boost::asio::any_io_executor executor_;
    // CURLM* multi_handle_;
    std::unique_ptr<CURLM, decltype(&curl_multi_cleanup)> multi_handle_;
    boost::asio::steady_timer timer_;

    std::mutex mutex_;
    std::map<CURL*, CompletionCallback> callbacks_;
    std::map<CURL*, curl_slist*> headers_;
    std::map<CURL*, std::shared_ptr<CURL>> handles_;
    std::map<curl_socket_t, SocketInfo> sockets_; // Managed socket info
    int still_running_{0};
};
} // namespace launchdarkly::network

#endif  // LD_CURL_NETWORKING