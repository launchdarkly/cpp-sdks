#include "backoff_timer.hpp"

#include <boost/asio/post.hpp>

namespace launchdarkly::sse {

BackoffTimer::BackoffTimer(boost::asio::any_io_executor executor)
    : executor_(std::move(executor)) {
    std::thread t([this]() { timer_thread_func(); });
    t.detach();
}

BackoffTimer::~BackoffTimer() {
    // Signal shutdown and wake up the timer thread
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
        cancelled_ = true;
    }
    cv_.notify_one();

    // Wait for the timer thread to complete
    // if (timer_thread_.joinable()) {
    //     timer_thread_.join();
    // }
}

void BackoffTimer::expires_after(std::chrono::milliseconds duration,
                                 std::function<void(bool cancelled)> callback) {
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Cancel any existing timer
        cancelled_ = timer_active_;

        // Set up the new timer
        expiry_time_ = std::chrono::steady_clock::now() + duration;
        callback_ = std::move(callback);
        timer_active_ = true;
        cancelled_ = false;
    }

    // Wake up the timer thread
    cv_.notify_one();
}

void BackoffTimer::cancel() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (timer_active_) {
            cancelled_ = true;
        }
    }

    // Wake up the timer thread
    cv_.notify_one();
}

boost::asio::any_io_executor BackoffTimer::get_executor() const {
    return executor_;
}

void BackoffTimer::timer_thread_func() {
    while (true) {
        std::function<void(bool cancelled)> callback_to_invoke;
        bool should_invoke = false;
        bool was_cancelled = false;

        {
            std::unique_lock<std::mutex> lock(mutex_);

            // Wait for either:
            // 1. A timer to be set (timer_active_ becomes true)
            // 2. Shutdown signal
            while (!timer_active_ && !shutdown_) {
                cv_.wait(lock);
            }

            // Check if we're shutting down
            if (shutdown_) {
                return;
            }

            // Wait until the timer expires or is cancelled
            while (timer_active_ && !cancelled_ && !shutdown_) {
                auto now = std::chrono::steady_clock::now();
                if (now >= expiry_time_) {
                    // Timer expired
                    should_invoke = true;
                    was_cancelled = false;
                    callback_to_invoke = std::move(callback_);
                    timer_active_ = false;
                    break;
                }

                // Wait until expiry time or until notified
                cv_.wait_until(lock, expiry_time_);
            }

            // Check if timer was cancelled
            if (cancelled_ && timer_active_) {
                should_invoke = true;
                was_cancelled = true;
                callback_to_invoke = std::move(callback_);
                timer_active_ = false;
                cancelled_ = false;
            }

            // Check if we're shutting down
            if (shutdown_) {
                return;
            }
        }

        // Invoke callback outside of lock by posting to the executor
        if (should_invoke && callback_to_invoke) {
            boost::asio::post(executor_, [callback = std::move(callback_to_invoke),
                                         was_cancelled]() {
                callback(was_cancelled);
            });
        }
    }
}

} // namespace launchdarkly::sse
