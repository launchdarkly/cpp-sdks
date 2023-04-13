#pragma once
#include <boost/container_hash/hash.hpp>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include "events/events.hpp"
#include "value.hpp"

namespace launchdarkly::events::detail {

class Summarizer {
   public:
    using Date = std::chrono::system_clock::time_point;
    using FlagKey = std::string;

    explicit Summarizer(Date start);
    Summarizer();
    void update(client::FeatureEventParams const& event);

    bool empty() const;

    Date start_time() const;

    struct VariationSummary {
        std::size_t count;
        Value value;
        explicit VariationSummary(Value value);
        void Increment();
    };

    struct VariationKey {
        std::optional<Version> version;
        std::optional<VariationIndex> variation;
        VariationKey(Version version, std::optional<VariationIndex> variation);

        VariationKey();

        bool operator==(VariationKey const& k) const {
            return k.variation == variation && k.version == version;
        }

        struct Hash {
            auto operator()(VariationKey const& p) const -> size_t {
                std::size_t seed = 0;
                boost::hash_combine(seed, p.version);
                boost::hash_combine(seed, p.variation);
                return seed;
            }
        };
    };

    struct State {
        Value default_;
        std::unordered_set<std::string> context_kinds;
        std::unordered_map<Summarizer::VariationKey,
                           Summarizer::VariationSummary,
                           Summarizer::VariationKey::Hash>
            counters;

        State(Value defaultVal);
    };

    std::unordered_map<FlagKey, State> const& features() const;

   private:
    Date start_time_;
    std::unordered_map<FlagKey, State> features_;
};

struct Summary {
    Summarizer const& summarizer;
    Summarizer::Date end_time;
};

}  // namespace launchdarkly::events::detail
