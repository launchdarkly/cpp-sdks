#pragma once

#include <chrono>
#include <cstddef>
namespace launchdarkly::config::detail {

enum class AttributePolicy {
    Default = 0,
    AllPrivate = 1,
};
struct Events {
    std::size_t capacity;
    std::chrono::milliseconds flush_interval;
    AttributePolicy attribute_policy;
};
}  // namespace launchdarkly::config::detail
