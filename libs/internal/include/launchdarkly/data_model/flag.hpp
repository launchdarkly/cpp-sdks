#pragma once

#include <launchdarkly/data/evaluation_reason.hpp>
#include <launchdarkly/data_model/context_aware_reference.hpp>
#include <launchdarkly/data_model/context_kind.hpp>
#include <launchdarkly/data_model/rule_clause.hpp>
#include <launchdarkly/value.hpp>

#include <boost/json/value.hpp>
#include <tl/expected.hpp>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace launchdarkly::data_model {

struct Flag {
    using Variation = std::int64_t;
    using Weight = std::int64_t;
    using FlagVersion = std::uint64_t;
    using Date = std::uint64_t;

    struct Rollout {
        enum class Kind {
            kUnrecognized = 0,
            kExperiment = 1,
            kRollout = 2,
        };

        struct WeightedVariation {
            Variation variation;
            Weight weight;
            bool untracked;

            WeightedVariation();

            WeightedVariation(Variation index, Weight weight);
            static WeightedVariation Untracked(Variation index, Weight weight);

           private:
            WeightedVariation(Variation index, Weight weight, bool untracked);
        };

        std::vector<WeightedVariation> variations;

        Kind kind;
        std::optional<std::int64_t> seed;

        DEFINE_ATTRIBUTE_REFERENCE_FIELD(bucketBy)
        DEFINE_CONTEXT_KIND_FIELD(contextKind)

        Rollout() = default;
        explicit Rollout(std::vector<WeightedVariation>);
    };

    using VariationOrRollout = std::variant<std::optional<Variation>, Rollout>;

    struct Prerequisite {
        std::string key;
        Variation variation;
    };

    struct Target {
        std::vector<std::string> values;
        Variation variation;
        ContextKind contextKind;
    };

    struct Rule {
        std::vector<Clause> clauses;
        VariationOrRollout variationOrRollout;

        bool trackEvents;
        std::optional<std::string> id;
    };

    struct ClientSideAvailability {
        bool usingMobileKey;
        bool usingEnvironmentId;
    };

    std::string key;
    FlagVersion version;
    bool on;
    VariationOrRollout fallthrough;
    std::vector<Value> variations;

    std::vector<Prerequisite> prerequisites;
    std::vector<Target> targets;
    std::vector<Target> contextTargets;
    std::vector<Rule> rules;
    std::optional<Variation> offVariation;
    bool clientSide;
    ClientSideAvailability clientSideAvailability;
    std::optional<std::string> salt;
    bool trackEvents;
    bool trackEventsFallthrough;
    std::optional<Date> debugEventsUntilDate;

    /**
     * Returns the flag's version. Satisfies ItemDescriptor template
     * constraints.
     * @return Version of this flag.
     */
    [[nodiscard]] inline std::uint64_t Version() const { return version; }

    [[nodiscard]] bool IsExperimentationEnabled(
        std::optional<EvaluationReason> const& reason) const;
};
}  // namespace launchdarkly::data_model
