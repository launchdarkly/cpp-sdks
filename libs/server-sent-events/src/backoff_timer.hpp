#pragma once

#include <boost/asio/any_io_executor.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace launchdarkly::sse {

/**
 * A thread-based timer that waits for a specified duration and then posts
 * a callback to an ASIO executor. The timer can be cancelled, making it
 * suitable for use in backoff scenarios where cleanup is required during
 * destruction.
 *
 * The timer uses a dedicated thread for waiting (via condition_variable)
 * and posts the callback to the provided ASIO executor when the timer expires.
 * This avoids blocking the ASIO thread pool during backoff periods.
 */
class BackoffTimer {
public:
    /**
     * Construct a BackoffTimer with the given ASIO executor.
     * @param executor The ASIO executor to post callbacks to when timer expires.
     */
    explicit BackoffTimer(boost::asio::any_io_executor executor);

    /**
     * Destructor. Cancels any pending timer and waits for the timer thread
     * to complete.
     */
    ~BackoffTimer();

    // Non-copyable and non-movable
    BackoffTimer(const BackoffTimer&) = delete;
    BackoffTimer& operator=(const BackoffTimer&) = delete;
    BackoffTimer(BackoffTimer&&) = delete;
    BackoffTimer& operator=(BackoffTimer&&) = delete;

    /**
     * Start an asynchronous wait. When the duration expires, the callback
     * will be posted to the executor provided in the constructor.
     *
     * If a timer is already running, it will be cancelled before starting
     * the new timer.
     *
     * @param duration The duration to wait before invoking the callback.
     * @param callback The callback to invoke when the timer expires.
     *                 The callback receives a boolean indicating whether
     *                 the timer was cancelled (true) or expired normally (false).
     */
    void expires_after(std::chrono::milliseconds duration,
                      std::function<void(bool cancelled)> callback);

    /**
     * Cancel any pending timer. If a timer is running, the callback will
     * be invoked with cancelled=true.
     */
    void cancel();

    /**
     * Get the executor used by this timer.
     */
    boost::asio::any_io_executor get_executor() const;

private:
    void timer_thread_func();

    boost::asio::any_io_executor executor_;

    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> shutdown_{false};
    std::atomic<bool> cancelled_{false};

    std::chrono::steady_clock::time_point expiry_time_;
    std::function<void(bool cancelled)> callback_;
    bool timer_active_{false};

    std::thread timer_thread_;
};

} // namespace launchdarkly::sse
