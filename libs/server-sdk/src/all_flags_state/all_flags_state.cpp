#include "launchdarkly/server_side/all_flags_state.hpp"

namespace launchdarkly::server_side {

AllFlagsState::State::State(
    std::uint64_t const version,
    std::optional<std::int64_t> const variation,
    std::optional<EvaluationReason> reason,
    bool const track_events,
    bool const track_reason,
    std::optional<std::uint64_t> const debug_events_until_date)
    : State(version,
            variation,
            std::move(reason),
            track_events,
            track_reason,
            debug_events_until_date,
            std::vector<std::string>{}) {}

AllFlagsState::State::State(
    std::uint64_t const version,
    std::optional<std::int64_t> const variation,
    std::optional<EvaluationReason> reason,
    bool const track_events,
    bool const track_reason,
    std::optional<std::uint64_t> const debug_events_until_date,
    std::vector<std::string> prerequisites)
    : version_(version),
      variation_(variation),
      reason_(std::move(reason)),
      track_events_(track_events),
      track_reason_(track_reason),
      debug_events_until_date_(debug_events_until_date),
      omit_details_(false),
      prerequisites_(std::move(prerequisites)) {}

std::uint64_t AllFlagsState::State::Version() const {
    return version_;
}

std::optional<std::int64_t> AllFlagsState::State::Variation() const {
    return variation_;
}

std::optional<EvaluationReason> const& AllFlagsState::State::Reason() const {
    return reason_;
}

bool AllFlagsState::State::TrackEvents() const {
    return track_events_;
}

bool AllFlagsState::State::TrackReason() const {
    return track_reason_;
}

std::vector<std::string> const& AllFlagsState::State::Prerequisites() const {
    return prerequisites_;
}

std::optional<std::uint64_t> const& AllFlagsState::State::DebugEventsUntilDate()
    const {
    return debug_events_until_date_;
}

bool AllFlagsState::State::OmitDetails() const {
    return omit_details_;
}

AllFlagsState::AllFlagsState()
    : valid_(false), evaluations_(), flags_state_() {}

AllFlagsState::AllFlagsState(std::unordered_map<std::string, Value> evaluations,
                             std::unordered_map<std::string, State> flags_state)
    : valid_(true),
      evaluations_(std::move(evaluations)),
      flags_state_(std::move(flags_state)) {}

bool AllFlagsState::Valid() const {
    return valid_;
}

std::unordered_map<std::string, AllFlagsState::State> const&
AllFlagsState::States() const {
    return flags_state_;
}

std::unordered_map<std::string, Value> const& AllFlagsState::Values() const {
    return evaluations_;
}

bool operator==(AllFlagsState const& lhs, AllFlagsState const& rhs) {
    return lhs.Valid() == rhs.Valid() && lhs.Values() == rhs.Values() &&
           lhs.States() == rhs.States();
}

bool operator==(AllFlagsState::State const& lhs,
                AllFlagsState::State const& rhs) {
    return lhs.Version() == rhs.Version() &&
           lhs.Variation() == rhs.Variation() && lhs.Reason() == rhs.Reason() &&
           lhs.TrackEvents() == rhs.TrackEvents() &&
           lhs.TrackReason() == rhs.TrackReason() &&
           lhs.DebugEventsUntilDate() == rhs.DebugEventsUntilDate() &&
           lhs.OmitDetails() == rhs.OmitDetails() &&
           lhs.Prerequisites() == rhs.Prerequisites();
}

}  // namespace launchdarkly::server_side
