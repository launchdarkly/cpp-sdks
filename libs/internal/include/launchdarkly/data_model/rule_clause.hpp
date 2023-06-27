#pragma once
#include <launchdarkly/attribute_reference.hpp>
#include <launchdarkly/value.hpp>
#include <optional>
#include <string>
#include <vector>

namespace launchdarkly::data_model {
struct Clause {
    constexpr static char const* kReferenceField = "attribute";
    constexpr static char const* kContextKindField = "contextKind";
    using ReferenceType = ContextAwareReference<Clause>;

    enum class Op {
        kUnrecognized, /* didn't match any known operators */
        kUnspecified,  /* the operator was not specified, i.e. omitted or empty
                          string */
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

    std::optional<AttributeReference> attribute;
    Op op;
    std::vector<Value> values;

    bool negate;
    std::optional<std::string> contextKind;
};
}  // namespace launchdarkly::data_model
