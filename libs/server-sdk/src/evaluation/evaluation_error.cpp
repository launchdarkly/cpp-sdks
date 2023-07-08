#include "evaluation_error.hpp"

namespace launchdarkly::server_side::evaluation {

std::ostream& operator<<(std::ostream& out, Error const& err) {
    switch (err) {
        case Error::kCyclicReference:
            return out << "cyclicReference";
        case Error::kBigSegmentEncountered:
            return out << "bigSegmentEncountered";
        case Error::kInvalidAttributeReference:
            return out << "invalidAttributeReference";
        case Error::kRolloutMissingVariations:
            return out << "rolloutMissingVariations";
        case Error::kUnknownOperator:
            return out << "unknownOperator";
    }
    return out << "unknownError";
}
}  // namespace launchdarkly::server_side::evaluation
