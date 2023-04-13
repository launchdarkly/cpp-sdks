#pragma once
#include <boost/container_hash/hash.hpp>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include "events/events.hpp"
#include "value.hpp"

namespace launchdarkly::events::detail {

/**
 * Summarizer is responsible for accepting FeatureEventParams (the context
 * related to a feature evaluation) and outputting summary events (which
 * essentially condenses the various evaluation results into a single
 * structure).
 */
class Summarizer {
   public:
    using Time = std::chrono::system_clock::time_point;
    using FlagKey = std::string;

    /**
     * Construct a Summarizer starting at the given time.
     * @param start Start time of the summary.
     */
    explicit Summarizer(Time start_time);

    /**
     * Construct a Summarizer at time zero.
     */
    Summarizer() = default;

    /**
     * Updates the summary with a feature event.
     * @param event Feature event.
     */
    void Update(client::FeatureEventParams const& event);

    /**
     * Marks the summary as finished at a given timestamp.
     * @param end_time End time of the summary.
     */
    Summarizer& Finish(Time end_time);

    /**
     * Returns true if the summary is empty.
     */
    [[nodiscard]] bool Empty() const;

    /**
     * Returns the summary's start time as given in the constructor.
     */
    [[nodiscard]] Time start_time() const;

    /**
     * Returns the summary's end time as specified using Finish.
     */
    [[nodiscard]] Time end_time() const;

    struct VariationSummary {
       public:
        explicit VariationSummary(Value value);
        void Increment();
        [[nodiscard]] std::int32_t count() const;
        [[nodiscard]] Value const& value() const;

       private:
        std::int32_t count_;
        Value value_;
    };

    struct VariationKey {
        std::optional<Version> version;
        std::optional<VariationIndex> variation;

        VariationKey();
        VariationKey(Version version, std::optional<VariationIndex> variation);

        bool operator==(VariationKey const& k) const {
            return k.variation == variation && k.version == version;
        }

        struct Hash {
            auto operator()(VariationKey const& key) const -> size_t {
                std::size_t seed = 0;
                boost::hash_combine(seed, key.version);
                boost::hash_combine(seed, key.variation);
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

        explicit State(Value defaultVal);
    };

    [[nodiscard]] std::unordered_map<FlagKey, State> const& features() const;

   private:
    Time start_time_;
    Summarizer::Time end_time_;
    std::unordered_map<FlagKey, State> features_;
};

}  // namespace launchdarkly::events::detail
