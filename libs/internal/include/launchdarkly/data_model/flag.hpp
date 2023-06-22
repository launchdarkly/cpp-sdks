#pragma once

#include <boost/json/value.hpp>
#include <launchdarkly/data_model/rule_clause.hpp>
#include <launchdarkly/value.hpp>
#include <optional>
#include <string>
#include <tl/expected.hpp>
#include <unordered_map>
#include <vector>

namespace launchdarkly::data_model {

struct Flag {
    struct Rollout {
        enum class Kind {
            kUnrecognized = 0,
            kExperiment = 1,
        };

        struct WeightedVariation {
            std::uint64_t variation;
            std::uint64_t weight;
            std::optional<bool> untracked;
        };

        std::vector<WeightedVariation> variations;

        std::optional<std::string> contextKind;
        std::optional<Kind> kind;
        std::optional<AttributeReference> bucketBy;
        std::optional<std::int64_t> seed;
    };

    using VariationOrRollout = std::variant<std::uint64_t, Rollout>;

    struct Prerequisite {
        std::string key;
        std::uint64_t variation;
    };

    struct Target {
        std::vector<std::string> values;
        std::uint64_t variation;
        std::optional<std::string> contextKind;
    };

    struct Rule {
        std::vector<Clause> clauses;
        VariationOrRollout variation_or_rollout;

        std::optional<bool> trackEvents;
        std::optional<std::string> id;
    };

    struct ClientSideAvailability {
        std::optional<bool> usingMobileKey;
        std::optional<bool> usingEnvironmentId;
    };

    std::string key;
    std::uint64_t version;
    bool on;
    VariationOrRollout fallthrough;
    std::vector<Value> variations;

    std::optional<std::vector<Prerequisite>> prerequisites;
    std::optional<std::vector<Target>> targets;
    std::optional<std::vector<Target>> contextTargets;
    std::optional<std::vector<Rule>> rules;
    std::optional<std::uint64_t> offVariation;
    std::optional<bool> clientSide;
    std::optional<ClientSideAvailability> clientSideAvailability;
    std::optional<std::string> salt;
    std::optional<bool> trackEvents;
    std::optional<bool> trackEventsFallthrough;
    std::optional<std::uint64_t> debugEventsUntilDate;
};
}  // namespace launchdarkly::data_model
