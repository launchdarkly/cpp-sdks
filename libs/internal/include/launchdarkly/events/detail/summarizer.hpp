#pragma once
#include <boost/container_hash/hash.hpp>

#include <chrono>
#include <functional>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "launchdarkly/value.hpp"

#include "launchdarkly/events/data/events.hpp"

namespace launchdarkly::events {

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
    void Update(events::FeatureEventParams const& event);

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
        explicit VariationSummary(::launchdarkly::Value value);
        void Increment();
        [[nodiscard]] std::int32_t Count() const;
        [[nodiscard]] ::launchdarkly::Value const& Value() const;

       private:
        std::int32_t count_;
        ::launchdarkly::Value value_;
    };

    struct VariationKey {
        std::optional<Version> version;
        std::optional<VariationIndex> variation;

        VariationKey();
        VariationKey(std::optional<Version> version,
                     std::optional<VariationIndex> variation);

        bool operator==(VariationKey const& k) const {
            return k.variation == variation && k.version == version;
        }

        bool operator<(VariationKey const& k) const {
            if (variation < k.variation) {
                return true;
            }
            return version < k.version;
        }
    };

    struct State {
        Value default_;
        std::set<std::string> context_kinds;
        std::map<Summarizer::VariationKey, Summarizer::VariationSummary>
            counters;

        explicit State(Value defaultVal);
    };

    [[nodiscard]] std::unordered_map<FlagKey, State> const& Features() const;

   private:
    Time start_time_;
    Summarizer::Time end_time_;
    std::unordered_map<FlagKey, State> features_;
};

}  // namespace launchdarkly::events
