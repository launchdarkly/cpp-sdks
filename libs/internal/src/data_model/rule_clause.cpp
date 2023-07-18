#include <launchdarkly/data_model/rule_clause.hpp>

namespace launchdarkly::data_model {

std::ostream& operator<<(std::ostream& os, Clause::Op operator_) {
    switch (operator_) {
        case Clause::Op::kUnrecognized:
            os << "unrecognized";
            break;
        case Clause::Op::kIn:
            os << "in";
            break;
        case Clause::Op::kStartsWith:
            os << "startsWith";
            break;
        case Clause::Op::kEndsWith:
            os << "endsWith";
            break;
        case Clause::Op::kMatches:
            os << "matches";
            break;
        case Clause::Op::kContains:
            os << "contains";
            break;
        case Clause::Op::kLessThan:
            os << "lessThan";
            break;
        case Clause::Op::kLessThanOrEqual:
            os << "lessThanOrEqual";
            break;
        case Clause::Op::kGreaterThan:
            os << "greaterThan";
            break;
        case Clause::Op::kGreaterThanOrEqual:
            os << "greaterThanOrEqual";
            break;
        case Clause::Op::kBefore:
            os << "before";
            break;
        case Clause::Op::kAfter:
            os << "after";
            break;
        case Clause::Op::kSemVerEqual:
            os << "semVerEqual";
            break;
        case Clause::Op::kSemVerLessThan:
            os << "semVerLessThan";
            break;
        case Clause::Op::kSemVerGreaterThan:
            os << "semVerGreaterThan";
            break;
        case Clause::Op::kSegmentMatch:
            os << "segmentMatch";
            break;
        default:
            os << "unknown";
            break;
    }
    return os;
}

}  // namespace launchdarkly::data_model
