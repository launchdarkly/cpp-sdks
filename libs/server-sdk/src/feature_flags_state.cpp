#include <launchdarkly/server_side/feature_flags_state.hpp>

namespace launchdarkly::server_side {
FeatureFlagsState::FeatureFlagsState()
    : valid_(false), evaluations_(), flags_state_() {}

FeatureFlagsState::FeatureFlagsState(
    std::unordered_map<std::string, Value> evaluations,
    std::unordered_map<std::string, FlagState> flags_state)
    : valid_(true),
      evaluations_(std::move(evaluations)),
      flags_state_(std::move(flags_state)) {}

bool FeatureFlagsState::Valid() const {
    return valid_;
}

std::unordered_map<std::string, FeatureFlagsState::FlagState> const&
FeatureFlagsState::FlagsState() const {
    return flags_state_;
}

std::unordered_map<std::string, Value> const& FeatureFlagsState::Evaluations()
    const {
    return evaluations_;
}

bool operator==(FeatureFlagsState const& lhs, FeatureFlagsState const& rhs) {
    return lhs.Valid() == rhs.Valid() &&
           lhs.Evaluations() == rhs.Evaluations() &&
           lhs.FlagsState() == rhs.FlagsState();
}

bool operator==(FeatureFlagsState::FlagState const& lhs,
                FeatureFlagsState::FlagState const& rhs) {
    return lhs.version == rhs.version && lhs.variation == rhs.variation &&
           lhs.reason == rhs.reason && lhs.track_events == rhs.track_events &&
           lhs.track_reason == rhs.track_reason &&
           lhs.debug_events_until_date == rhs.debug_events_until_date;
}

}  // namespace launchdarkly::server_side
