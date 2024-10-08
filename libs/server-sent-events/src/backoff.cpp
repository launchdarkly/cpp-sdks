#include "backoff.hpp"
#include "backoff_detail.hpp"

static constexpr double kDefaultJitterRatio = 0.5;
static constexpr auto kDefaultResetInterval = std::chrono::milliseconds{60'000};

namespace launchdarkly::sse {

std::chrono::milliseconds Backoff::delay() const {
    return detail::delay(initial_, max_, attempt_, max_exponent_, jitter_ratio_,
                         random_);
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
    : initial_(initial),
      max_(max),
      // This is necessary to constrain the exponent that is passed eventually
      // into std::pow. Otherwise, we'll be operating with a number that is too
      // large to be represented as a chrono::milliseconds and (depending on the
      // platform) may result in a value of 0, negative, or some other
      // unexpected value.
      max_exponent_(std::ceil(
          std::log2(max.count() / std::max(std::chrono::milliseconds{1}.count(),
                                           initial.count())))),
      attempt_(1),
      jitter_ratio_(jitter_ratio),
      reset_interval_(reset_interval),
      random_(std::move(random)) {}

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
}  // namespace launchdarkly::sse
