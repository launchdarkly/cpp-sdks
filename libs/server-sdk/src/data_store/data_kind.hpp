#pragma once

#include <cstddef>

namespace launchdarkly::server_side::data_store {
enum class DataKind : std::size_t { kFlag = 0, kSegment = 1, kKindCount = 2 };
}
