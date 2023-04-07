#pragma once

#include <bits/stdc++.h>
#include <chrono>
#include <optional>

namespace launchdarkly::sse::detail {

/**
 * Implements an algorithm for calculating the delay between connection
 * attempts.
 */
class Backoff {
   public:
    /**
     *
     * @param initial Initial delay for the first failed connection.
     * @param max The maximum delay between retries.
     */
    Backoff(std::chrono::duration<uint64_t, std::milli> initial,
            std::chrono::duration<uint64_t, std::milli> max,
            double jitter_ratio,
            std::chrono::milliseconds reset_interval,
            std::default_random_engine gen);

    Backoff(std::chrono::duration<uint64_t, std::milli> initial,
            std::chrono::duration<uint64_t, std::milli> max);

    /**
     * Report to the backoff that their was a connection failure.
     */
    void fail();

    /**
     * Report to the backoff that there was a successful connection.
     */
    void succeed();

    /**
     * Get the current reconnect delay.
     */
    std::chrono::milliseconds delay() const;

   private:
    /**
     * Calculate an exponential backoff based on the initial retry time and
     * the current attempt.
     *
     * @param attempt The current attempt count.
     * @param initial The initial retry delay.
     * @return The current backoff based on the number of attempts.
     */
    std::chrono::milliseconds CalculateBackoff();

    /**
     * Produce a jittered version of the base value. This value will be adjusted
     * randomly to be between 50% and 100% of the input duration.
     *
     * @param base The base duration to jitter.
     * @return The jittered duration.
     */
    std::chrono::milliseconds Jitter(std::chrono::milliseconds base);

    /**
     * Given an attempt count, the initial delay, and the maximum delay produce
     * the current delay.
     *
     * @return A new delay incorporating jitter and backoff.
     */
    std::chrono::milliseconds Delay(uint64_t attempt,
                                    std::chrono::milliseconds initial,
                                    std::chrono::milliseconds max);

    std::chrono::milliseconds initial_;
    std::chrono::milliseconds current_;
    std::chrono::milliseconds max_;
    uint64_t attempt_;
    std::optional<std::chrono::time_point<std::chrono::system_clock>>
        active_since_;
    std::default_random_engine random_gen_;
    double const jitter_ratio_;
    const std::chrono::milliseconds reset_interval_;
};
}  // namespace launchdarkly::sse::detail
