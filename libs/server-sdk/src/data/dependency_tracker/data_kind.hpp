#pragma once

#include <cstddef>

namespace launchdarkly::server_side::data {
enum class DataKind : std::size_t { kFlag = 0, kSegment = 1, kKindCount = 2 };
}  // namespace launchdarkly::server_side::data
