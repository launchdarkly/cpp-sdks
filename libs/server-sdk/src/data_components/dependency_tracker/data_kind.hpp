#pragma once

#include <launchdarkly/detail/unreachable.hpp>

#include <cstddef>
#include <ostream>

namespace launchdarkly::server_side::data_components {
enum class DataKind : std::size_t { kFlag = 0, kSegment = 1, kKindCount = 2 };

inline std::ostream& operator<<(std::ostream& out, DataKind const& kind) {
    switch (kind) {
        case DataKind::kFlag:
            out << "flag";
            return out;
        case DataKind::kSegment:
            out << "segment";
            return out;
        case DataKind::kKindCount:
            out << "kind_count";
            return out;
    }
    detail::unreachable();
}

}  // namespace launchdarkly::server_side::data_components
