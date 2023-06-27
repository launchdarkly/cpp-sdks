#pragma once
#include <launchdarkly/attribute_reference.hpp>
#include <launchdarkly/value.hpp>
#include <optional>
#include <string>
#include <vector>

namespace launchdarkly::data_model {
struct Clause {
    enum class Op {
        kUnrecognized, /* didn't match any known operators */
        kIn,
        kStartsWith,
        kEndsWith,
        kMatches,
        kContains,
        kLessThan,
        kLessThanOrEqual,
        kGreaterThan,
        kGreaterThanOrEqual,
        kBefore,
        kAfter,
        kSemVerEqual,
        kSemVerLessThan,
        kSemVerGreaterThan,
        kSegmentMatch
    };

    Op op;
    std::vector<Value> values;
    bool negate;

    DEFINE_CONTEXT_KIND_FIELD(contextKind)
    DEFINE_ATTRIBUTE_REFERENCE_FIELD(attribute)
};
}  // namespace launchdarkly::data_model
