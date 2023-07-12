#pragma once

#include <ostream>

namespace launchdarkly::server_side::evaluation {

enum class Error {
    /* A cyclic reference was detected within flags or segments. */
    kCyclicReference,
    /* A big segment was encountered; big segments are not supported in this
     * SDK. */
    kBigSegmentEncountered,
    /* Encountered an invalid attribute reference. */
    kInvalidAttributeReference,
    /* A rollout was missing variations. */
    kRolloutMissingVariations,
    /* An operator was supplied that isn't recognized by this SDK. */
    kUnrecognizedOperator,
};

std::ostream& operator<<(std::ostream& out, Error const& arr);

}  // namespace launchdarkly::server_side::evaluation
