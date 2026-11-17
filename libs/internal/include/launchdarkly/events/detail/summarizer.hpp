#pragma once
#include <boost/container_hash/hash.hpp>

#include <chrono>
#include <functional>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "launchdarkly/value.hpp"

#include "launchdarkly/events/data/events.hpp"

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
     * Construct an empty Summarizer. Its start and end times are zero.
     */
    Summarizer() = default;

    /**
     * Updates the summary with a feature event. The start and end times of
     * the summary become the earliest and latest event creation times.
     * @param event Feature event.
     */
    void Update(events::FeatureEventParams const& event);

    /**
     * Returns true if the summary is empty.
     */
    [[nodiscard]] bool Empty() const;

    /**
     * Returns the creation time of the earliest event in the summary.
     */
    [[nodiscard]] Time StartTime() const;

    /**
     * Returns the creation time of the latest event in the summary.
     */
    [[nodiscard]] Time EndTime() const;

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
            return !(*this < k) && !(k < *this);
        }

        bool operator!=(VariationKey const& k) const { return !(*this == k); }

        bool operator<(VariationKey const& k) const {
            if (variation < k.variation) {
                return true;
            }
            if (variation > k.variation) {
                return false;
            }
            return version < k.version;
        }

        bool operator>(VariationKey const& k) const { return k < *this; }
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

}  // namespace launchdarkly::events::detail
