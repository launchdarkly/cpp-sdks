#pragma once

#include <boost/json/value.hpp>
#include <launchdarkly/attribute_reference.hpp>
#include <launchdarkly/data_model/rule_clause.hpp>
#include <launchdarkly/value.hpp>
#include <optional>
#include <string>
#include <tl/expected.hpp>
#include <unordered_map>
#include <vector>

namespace launchdarkly::data_model {

struct Segment {
    struct Target {
        std::string contextKind;
        std::vector<std::string> values;
    };
    struct Rule {
        std::vector<Clause> clauses;
        std::optional<std::string> id;
        std::optional<std::uint64_t> weight;
        std::optional<AttributeReference> bucketBy;
        std::optional<std::string> rolloutContextKind;
    };
    std::string key;
    std::uint64_t version;

    std::optional<std::vector<std::string>> included;
    std::optional<std::vector<std::string>> excluded;
    std::optional<std::vector<Target>> includedContexts;
    std::optional<std::vector<Target>> excludedContexts;
    std::optional<std::vector<Rule>> rules;
    std::optional<std::string> salt;
    std::optional<bool> unbounded;
    std::optional<std::string> unboundedContextKind;
    std::optional<std::uint64_t> generation;

    [[nodiscard]] inline std::uint64_t Version() const { return version; }
};
}  // namespace launchdarkly::data_model
