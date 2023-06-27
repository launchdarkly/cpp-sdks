#pragma once

#include <boost/json/value.hpp>
#include <launchdarkly/data_model/context_aware_reference.hpp>
#include <launchdarkly/data_model/rule_clause.hpp>
#include <launchdarkly/value.hpp>
#include <optional>
#include <string>
#include <tl/expected.hpp>
#include <unordered_map>
#include <vector>

namespace launchdarkly::data_model {

struct Flag {
    using ContextKind = std::string;

    struct Rollout {
        constexpr static char const* kReferenceField = "bucketBy";
        constexpr static char const* kContextKindField = "contextKind";
        using ReferenceType = ContextAwareReference<Rollout>;

        enum class Kind {
            kUnrecognized = 0,
            kExperiment = 1,
            kRollout = 2,
        };

        struct WeightedVariation {
            std::uint64_t variation;
            std::uint64_t weight;
            bool untracked;
        };

        std::vector<WeightedVariation> variations;

        ContextKind contextKind;
        Kind kind;
        AttributeReference bucketBy;
        std::optional<std::int64_t> seed;
    };

    using Variation = std::uint64_t;
    using VariationOrRollout = std::variant<Variation, Rollout>;

    struct Prerequisite {
        std::string key;
        std::uint64_t variation;
    };

    struct Target {
        std::vector<std::string> values;
        std::uint64_t variation;
        ContextKind contextKind;
    };

    struct Rule {
        std::vector<Clause> clauses;
        VariationOrRollout variation_or_rollout;

        bool trackEvents;
        std::optional<std::string> id;
    };

    struct ClientSideAvailability {
        bool usingMobileKey;
        bool usingEnvironmentId;
    };

    std::string key;
    std::uint64_t version;
    bool on;
    VariationOrRollout fallthrough;
    std::vector<Value> variations;

    std::vector<Prerequisite> prerequisites;
    std::vector<Target> targets;
    std::vector<Target> contextTargets;
    std::vector<Rule> rules;
    std::uint64_t offVariation;
    bool clientSide;
    ClientSideAvailability clientSideAvailability;
    std::optional<std::string> salt;
    bool trackEvents;
    bool trackEventsFallthrough;
    std::optional<std::uint64_t> debugEventsUntilDate;
};
}  // namespace launchdarkly::data_model
