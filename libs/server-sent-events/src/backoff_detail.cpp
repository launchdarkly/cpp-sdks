#include "backoff_detail.hpp"

#include <cmath>

namespace launchdarkly::sse::detail {
std::chrono::milliseconds calculate_backoff(
    std::chrono::milliseconds const initial,
    std::uint64_t const attempt,
    std::uint64_t const max_exponent) {
    std::uint64_t const exponent = std::min(attempt - 1, max_exponent);
    double const exponentiated = std::pow(2, exponent);
    return initial * static_cast<uint64_t>(exponentiated);
}

std::chrono::milliseconds jitter(std::chrono::milliseconds const base,
                                 double const jitter_ratio,
                                 std::function<double(double)> const& random) {
    double const jitter = random(jitter_ratio);
    double const jittered_base = base.count() - (jitter * base.count());
    return std::chrono::milliseconds(static_cast<uint64_t>(jittered_base));
}

std::chrono::milliseconds delay(std::chrono::milliseconds const initial,
                                std::chrono::milliseconds const max,
                                std::uint64_t const attempt,
                                std::uint64_t const max_exponent,
                                double const jitter_ratio,
                                std::function<double(double)> const& random) {
    auto const backoff = calculate_backoff(initial, attempt, max_exponent);
    auto const constrained_backoff = std::min(backoff, max);
    return jitter(constrained_backoff, jitter_ratio, random);
}
}  // namespace launchdarkly::sse::detail
