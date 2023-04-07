#include "launchdarkly/sse/detail/backoff.hpp"

static double const kDefaultJitterRatio = 0.5;
static const std::chrono::milliseconds kDefaultResetInterval =
    std::chrono::milliseconds{60'000};

namespace launchdarkly::sse::detail {

std::chrono::milliseconds Backoff::CalculateBackoff() {
    return initial_ * (2 ^ (attempt_ - 1));
}

std::chrono::milliseconds Backoff::Jitter(std::chrono::milliseconds base) {
    std::uniform_real_distribution<double> distribution(0.0, jitter_ratio_);
    return std::chrono::milliseconds(static_cast<uint64_t>(
        base.count() - (distribution(random_gen_) * base.count())));
}

std::chrono::milliseconds Backoff::Delay(uint64_t attempt,
                                         std::chrono::milliseconds initial,
                                         std::chrono::milliseconds max) {
    return Jitter(std::min(CalculateBackoff(), max));
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
    // Calculate a new delay based on the updated attempt count.
    current_ = Delay(attempt_, initial_, max_);
    // There has been a failure, so the connection is no longer active.
    active_since_ = std::nullopt;
}

void Backoff::succeed() {
    active_since_ = std::chrono::system_clock::now();
}

std::chrono::milliseconds Backoff::delay() const {
    return current_;
}

Backoff::Backoff(std::chrono::duration<uint64_t, std::milli> initial,
                 std::chrono::duration<uint64_t, std::milli> max,
                 double jitter_ratio,
                 std::chrono::milliseconds reset_interval,
                 std::default_random_engine gen)
    : initial_(initial),
      max_(max),
      current_(initial),
      jitter_ratio_(jitter_ratio),
      reset_interval_(reset_interval),
      random_gen_(gen) {}

Backoff::Backoff(std::chrono::duration<uint64_t, std::milli> initial,
                 std::chrono::duration<uint64_t, std::milli> max)
    : Backoff(initial,
              max,
              kDefaultJitterRatio,
              kDefaultResetInterval,
              std::default_random_engine()) {}

}  // namespace launchdarkly::sse::detail