#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include <launchdarkly/data/evaluation_reason.hpp>
#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/value.hpp>

namespace launchdarkly::server_side {

class FeatureFlagsState {
   public:
    struct FlagState {
        std::optional<data_model::Flag::FlagVersion> version;
        std::optional<data_model::Flag::Variation> variation;
        std::optional<EvaluationReason> reason;
        bool track_events;
        bool track_reason;
        std::optional<data_model::Flag::Date> debug_events_until_date;
    };

    [[nodiscard]] bool Valid() const;

    [[nodiscard]] std::unordered_map<std::string, FlagState> const& FlagsState()
        const;
    [[nodiscard]] std::unordered_map<std::string, Value> const& Evaluations()
        const;

    /**
     * Creates an invalid instance of FeatureFlagsState.
     */
    FeatureFlagsState();

    FeatureFlagsState(std::unordered_map<std::string, Value> evaluations,
                      std::unordered_map<std::string, FlagState> flags_state);

   private:
    bool const valid_;
    const std::unordered_map<std::string, FlagState> flags_state_;
    const std::unordered_map<std::string, Value> evaluations_;
};

bool operator==(FeatureFlagsState::FlagState const& lhs,
                FeatureFlagsState::FlagState const& rhs);

bool operator==(FeatureFlagsState const& lhs, FeatureFlagsState const& rhs);

}  // namespace launchdarkly::server_side
