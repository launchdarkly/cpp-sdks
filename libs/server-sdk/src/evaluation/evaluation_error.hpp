#pragma once

#include <ostream>

namespace launchdarkly::server_side::evaluation {

enum class Error {
    kCyclicReference,
    kBigSegmentEncountered,
    kInvalidAttributeReference,
    kRolloutMissingVariations,
    kUnknownOperator,
};

std::ostream& operator<<(std::ostream& out, Error const& arr);

}  // namespace launchdarkly::server_side::evaluation
