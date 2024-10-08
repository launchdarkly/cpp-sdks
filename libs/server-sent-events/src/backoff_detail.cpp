#include "backoff_detail.hpp"

namespace launchdarkly::sse::detail {
std::chrono::milliseconds calculate_backoff(
    std::chrono::milliseconds const initial,
    std::uint64_t const attempt,
    std::uint64_t const max_exponent) {
    std::uint64_t const exponent = std::min(attempt - 1, max_exponent);
    return initial * static_cast<uint64_t>(std::pow(2, exponent));
}

std::chrono::milliseconds jitter(std::chrono::milliseconds const base,
                                 double const jitter_ratio,
                                 std::function<double(double)> const& random) {
    return std::chrono::milliseconds(static_cast<uint64_t>(
        base.count() - (random(jitter_ratio) * base.count())));
}

std::chrono::milliseconds delay(std::chrono::milliseconds const initial,
                                std::chrono::milliseconds const max,
                                std::uint64_t const attempt,
                                std::uint64_t const max_exponent,
                                double const jitter_ratio,
                                std::function<double(double)> const& random) {
    return jitter(
        std::min(calculate_backoff(initial, attempt, max_exponent), max),
        jitter_ratio, random);
}
}  // namespace detail
