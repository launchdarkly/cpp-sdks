#pragma once

#include <chrono>
#include <functional>
#include <optional>
#include <random>

namespace launchdarkly::sse {
/**
 * Implements an algorithm for calculating the delay between connection
 * attempts.
 */
class Backoff {
   public:
    /**
     * Construct a backoff instance with customized behavior.
     * @param initial Initial delay for the first failed connection.
     * @param max The maximum delay between retries.
     * @param jitter_ratio The ratio to use when jittering. The jitter
     * is of the form `CalculdatedBackoff - (rand(ratio) * CalculatedBackoff)`
     * with `rand` producing a uniform distrubution between 0 and the
     * jitter_ratio.
     * @param reset_interval The milliseconds interval to reset the backoff
     * attempts after a successful connection.
     * @param random A random method which produces a value between 0 and
     * jitter_ratio. Primarily intended for testing.
     */
    Backoff(std::chrono::milliseconds initial,
            std::chrono::milliseconds max,
            double jitter_ratio,
            std::chrono::milliseconds reset_interval,
            std::function<double(double ratio)> random);

    /**
     * Construct a backoff instance with default behavior.
     * @param initial Initial delay for the first failed connection.
     * @param max The maximum delay between retries.
     */
    Backoff(std::chrono::milliseconds initial, std::chrono::milliseconds max);

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
    std::chrono::milliseconds initial_;
    std::chrono::milliseconds max_;

    std::uint64_t max_exponent_;
    std::uint64_t attempt_;
    std::optional<std::chrono::time_point<std::chrono::system_clock>>
        active_since_;
    double const jitter_ratio_;
    std::chrono::milliseconds const reset_interval_;
    // Default random generator. Used when random method not specified.
    std::function<double(double ratio)> random_;
    std::default_random_engine random_gen_;
};
}  // namespace launchdarkly::sse
