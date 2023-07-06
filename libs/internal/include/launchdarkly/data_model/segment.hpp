#pragma once

#include <launchdarkly/attribute_reference.hpp>
#include <launchdarkly/data_model/context_aware_reference.hpp>
#include <launchdarkly/data_model/rule_clause.hpp>
#include <launchdarkly/value.hpp>

#include <boost/json/value.hpp>
#include <tl/expected.hpp>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace launchdarkly::data_model {

struct Segment {
    using Kind = std::string;
    struct Target {
        Kind contextKind;
        std::vector<std::string> values;
    };

    struct Rule {
        using ReferenceType = ContextAwareReference<Rule>;

        std::vector<Clause> clauses;
        std::optional<std::string> id;
        std::optional<std::uint64_t> weight;

        DEFINE_CONTEXT_KIND_FIELD(rolloutContextKind)
        DEFINE_ATTRIBUTE_REFERENCE_FIELD(bucketBy)
    };

    std::string key;
    std::uint64_t version;

    std::vector<std::string> included;
    std::vector<std::string> excluded;
    std::vector<Target> includedContexts;
    std::vector<Target> excludedContexts;
    std::vector<Rule> rules;
    std::optional<std::string> salt;
    bool unbounded;
    std::optional<Kind> unboundedContextKind;
    std::optional<std::uint64_t> generation;

    // TODO(cwaldren): make Kind a real type that is deserialized, so we can
    // make empty string an error.
    [[nodiscard]] inline std::uint64_t Version() const { return version; }
};
}  // namespace launchdarkly::data_model
