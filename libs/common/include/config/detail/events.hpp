#pragma once

#include <chrono>
#include <cstddef>
namespace launchdarkly::config::detail {

struct Events {
    std::size_t capacity;
    std::chrono::milliseconds flush_interval;
};
}  // namespace launchdarkly::config::detail
