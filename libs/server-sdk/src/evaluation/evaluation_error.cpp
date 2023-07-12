#include "evaluation_error.hpp"

namespace launchdarkly::server_side::evaluation {

std::ostream& operator<<(std::ostream& out, Error const& err) {
    switch (err) {
        case Error::kCyclicReference:
            out << "CYCLIC_REFERENCE";
            break;
        case Error::kBigSegmentEncountered:
            out << "BIG_SEGMENT_ENCOUNTERED";
            break;
        case Error::kInvalidAttributeReference:
            out << "INVALID_ATTRIBUTE_REFERENCE";
            break;
        case Error::kRolloutMissingVariations:
            out << "ROLLOUT_MISSING_VARIATIONS";
            break;
        case Error::kUnrecognizedOperator:
            out << "UNRECOGNIZED_OPERATOR";
            break;
        default:
            out << "UNKNOWN_ERROR";
            break;
    }
    return out;
}
}  // namespace launchdarkly::server_side::evaluation
