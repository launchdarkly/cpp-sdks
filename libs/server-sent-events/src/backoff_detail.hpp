#pragma once

#include <chrono>
#include <functional>

namespace launchdarkly::sse::detail {
/**
 * Calculate an exponential backoff based on the initial retry time and
 * the current attempt.
 *
 * @param initial The initial retry delay.
 * @param attempt The current attempt count.
 * @param max_exponent The maximum exponent to use in the calculation.
 *
 * @return The current backoff based on the number of attempts.
 */

std::chrono::milliseconds calculate_backoff(std::chrono::milliseconds initial,
                                            std::uint64_t attempt,
                                            std::uint64_t max_exponent);
/**
 * Produce a jittered version of the base value. This value will be adjusted
 * randomly to be between 50% and 100% of the input duration.
 *
 * @param base The base duration to jitter.
 * @param jitter_ratio The ratio to use when jittering.
 * @param random A random method which produces a value between 0 and
 * jitter_ratio.
 * @return The jittered duration.
 */
std::chrono::milliseconds jitter(std::chrono::milliseconds base,
                                 double jitter_ratio,
                                 std::function<double(double)> const& random);

/**
 *
 * @param initial The initial retry delay.
 * @param max The maximum delay between retries.
 * @param attempt The current attempt.
 * @param max_exponent The maximum exponent to use in the calculation.
 * @param jitter_ratio The ratio to use when jittering.
 * @param random A random method which produces a value between 0 and
 * jitter_ratio.
 * @return The delay between connection attempts.
 */
std::chrono::milliseconds delay(std::chrono::milliseconds initial,
                                std::chrono::milliseconds max,
                                std::uint64_t attempt,
                                std::uint64_t max_exponent,
                                double jitter_ratio,
                                std::function<double(double)> const& random);
}  // namespace launchdarkly::sse::detail
