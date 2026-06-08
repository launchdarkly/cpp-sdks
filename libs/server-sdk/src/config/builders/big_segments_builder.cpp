#include "big_segments_builder.hpp"

#include <algorithm>
#include <chrono>
#include <utility>

namespace launchdarkly::server_side::config::builders {

namespace {

using namespace std::chrono_literals;

constexpr std::size_t kDefaultContextCacheSize = 1000;
constexpr std::chrono::milliseconds kDefaultContextCacheTime = 5s;
constexpr std::chrono::milliseconds kDefaultStatusPollInterval = 5s;
constexpr std::chrono::milliseconds kDefaultStaleAfter = 2min;

}  // namespace

BigSegmentsBuilder::BigSegmentsBuilder(
    std::shared_ptr<integrations::IBigSegmentStore> store)
    : store_(std::move(store)),
      context_cache_size_(kDefaultContextCacheSize),
      context_cache_time_(kDefaultContextCacheTime),
      status_poll_interval_(kDefaultStatusPollInterval),
      stale_after_(kDefaultStaleAfter) {}

BigSegmentsBuilder& BigSegmentsBuilder::ContextCacheSize(
    std::size_t const size) {
    context_cache_size_ = size;
    return *this;
}

BigSegmentsBuilder& BigSegmentsBuilder::ContextCacheTime(
    std::chrono::milliseconds const ttl) {
    context_cache_time_ = ttl > std::chrono::milliseconds::zero()
                              ? ttl
                              : kDefaultContextCacheTime;
    return *this;
}

BigSegmentsBuilder& BigSegmentsBuilder::StatusPollInterval(
    std::chrono::milliseconds const interval) {
    status_poll_interval_ = interval > std::chrono::milliseconds::zero()
                                ? interval
                                : kDefaultStatusPollInterval;
    return *this;
}

BigSegmentsBuilder& BigSegmentsBuilder::StaleAfter(
    std::chrono::milliseconds const threshold) {
    stale_after_ = threshold > std::chrono::milliseconds::zero()
                       ? threshold
                       : kDefaultStaleAfter;
    return *this;
}

built::BigSegmentsConfig BigSegmentsBuilder::Build() const {
    auto const poll = std::min(status_poll_interval_, stale_after_);
    return built::BigSegmentsConfig{store_, context_cache_size_,
                                    context_cache_time_, poll, stale_after_};
}

}  // namespace launchdarkly::server_side::config::builders
