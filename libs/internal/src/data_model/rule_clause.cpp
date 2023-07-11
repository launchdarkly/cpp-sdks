#include <launchdarkly/data_model/rule_clause.hpp>

namespace launchdarkly::data_model {

std::ostream& operator<<(std::ostream& os, Clause::Op op) {
    switch (op) {
        case Clause::Op::kUnrecognized:
            return os << "unrecognized";
        case Clause::Op::kIn:
            return os << "in";
        case Clause::Op::kStartsWith:
            return os << "startsWith";
        case Clause::Op::kEndsWith:
            return os << "endsWith";
        case Clause::Op::kMatches:
            return os << "matches";
        case Clause::Op::kContains:
            return os << "contains";
        case Clause::Op::kLessThan:
            return os << "lessThan";
        case Clause::Op::kLessThanOrEqual:
            return os << "lessThanOrEqual";
        case Clause::Op::kGreaterThan:
            return os << "greaterThan";
        case Clause::Op::kGreaterThanOrEqual:
            return os << "greaterThanOrEqual";
        case Clause::Op::kBefore:
            return os << "before";
        case Clause::Op::kAfter:
            return os << "after";
        case Clause::Op::kSemVerEqual:
            return os << "semVerEqual";
        case Clause::Op::kSemVerLessThan:
            return os << "semVerLessThan";
        case Clause::Op::kSemVerGreaterThan:
            return os << "semVerGreaterThan";
        case Clause::Op::kSegmentMatch:
            return os << "segmentMatch";
        default:
            return os << "unknown";
    }
}
}  // namespace launchdarkly::data_model
