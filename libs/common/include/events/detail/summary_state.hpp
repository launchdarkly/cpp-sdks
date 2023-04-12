#pragma once
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include "events/events.hpp"
#include "value.hpp"

namespace launchdarkly::events::detail {

class Summarizer {
   public:
    Summarizer(std::chrono::system_clock::time_point start);
    void update(client::FeatureEventParams const& event);

    struct VariationSummary {
        std::size_t count;
        Value value;
        VariationSummary() = default;
    };

    struct VariationKey {
        Version version;
        std::optional<VariationIndex> variation;
        VariationKey(Version version, std::optional<VariationIndex> variation);

        struct Hash {
            auto operator()(VariationKey const& p) const -> size_t {
                if (p.variation) {
                    return std::hash<Version>{}(p.version) ^
                           std::hash<VariationIndex>{}(*p.variation);
                } else {
                    return std::hash<Version>{}(p.version);
                }
            }
        };
    };

    struct State {
        Value default_;
        std::unordered_set<std::string> context_kinds_;
        std::unordered_map<VariationKey, VariationSummary, VariationKey::Hash>
            counters_;

        State(Value defaultVal, std::unordered_set<std::string> contextKinds);
    };

    using FlagKey = std::string;
    std::chrono::system_clock::time_point start_time_;
    std::chrono::system_clock::time_point end_time_;
    std::unordered_map<FlagKey, State> features_;
};

}  // namespace launchdarkly::events::detail
