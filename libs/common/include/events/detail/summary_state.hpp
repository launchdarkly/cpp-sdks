#pragma once
#include <chrono>
#include <functional>
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
        explicit VariationSummary(Value value);
        void Increment();
    };

    struct VariationKey {
        VariationKey(VariationKey const& key);
        std::optional<Version> version;
        std::optional<VariationIndex> variation;
        VariationKey(Version version, std::optional<VariationIndex> variation);

        VariationKey();

        bool operator==(VariationKey const& k) const {
            return k.variation == variation && k.version == version;
        }

        struct Hash {
            auto operator()(VariationKey const& p) const -> size_t {
                return std::hash<decltype(p.version)>{}(p.version) ^
                       std::hash<decltype(p.variation)>{}(p.variation);
            }
        };
    };

    struct State {
        Value default_;
        std::unordered_set<std::string> context_kinds_;
        std::unordered_map<Summarizer::VariationKey,
                           Summarizer::VariationSummary,
                           Summarizer::VariationKey::Hash>
            counters_;

        State(Value defaultVal);
    };

    using FlagKey = std::string;
    std::chrono::system_clock::time_point start_time_;
    std::chrono::system_clock::time_point end_time_;
    std::unordered_map<FlagKey, State> features_;
};

}  // namespace launchdarkly::events::detail
