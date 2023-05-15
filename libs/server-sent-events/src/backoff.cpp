#include <launchdarkly/sse/detail/backoff.hpp>

static double const kDefaultJitterRatio = 0.5;
static const std::chrono::milliseconds kDefaultResetInterval =
    std::chrono::milliseconds{60'000};

namespace launchdarkly::sse::detail {

std::chrono::milliseconds Backoff::calculate_backoff() {
    return initial_ * static_cast<uint64_t>(std::pow(2, attempt_ - 1));
}

std::chrono::milliseconds Backoff::jitter(std::chrono::milliseconds base) {
    return std::chrono::milliseconds(static_cast<uint64_t>(
        base.count() - (random_(jitter_ratio_) * base.count())));
}

std::chrono::milliseconds Backoff::delay() {
    return jitter(std::min(calculate_backoff(), max_));
}

void Backoff::fail() {
    // Active since being undefined means we have not had a successful
    // connection since the last attempt.
    if (active_since_.has_value()) {
        auto active_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - active_since_.value());
        // If the active duration is greater than the reset interval,
        // then reset the attempts to 1. Otherwise continue increasing
        // attempts.
        if (active_duration > reset_interval_) {
            attempt_ = 1;
        } else {
            attempt_ += 1;
        }
    } else {
        attempt_ += 1;
    }
    // There has been a failure, so the connection is no longer active.
    active_since_ = std::nullopt;
}

void Backoff::succeed() {
    active_since_ = std::chrono::system_clock::now();
}

Backoff::Backoff(std::chrono::milliseconds initial,
                 std::chrono::milliseconds max,
                 double jitter_ratio,
                 std::chrono::milliseconds reset_interval,
                 std::function<double(double ratio)> random)
    : attempt_(1),
      initial_(initial),
      max_(max),
      jitter_ratio_(jitter_ratio),
      reset_interval_(reset_interval),
      random_(random) {}

Backoff::Backoff(std::chrono::milliseconds initial,
                 std::chrono::milliseconds max)
    : Backoff(initial,
              max,
              kDefaultJitterRatio,
              kDefaultResetInterval,
              [this](auto ratio) {
                  std::uniform_real_distribution<double> distribution(
                      0.0, kDefaultJitterRatio);
                  return distribution(this->random_gen_);
              }) {}

}  // namespace launchdarkly::sse::detail
