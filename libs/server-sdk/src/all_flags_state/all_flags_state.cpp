#include "launchdarkly/server_side/all_flags_state.hpp"

namespace launchdarkly::server_side {

AllFlagsState::Metadata::Metadata(
    std::uint64_t version,
    std::optional<std::int64_t> variation,
    std::optional<EvaluationReason> reason,
    bool track_events,
    bool track_reason,
    std::optional<std::uint64_t> debug_events_until_date)
    : version_(version),
      variation_(variation),
      reason_(reason),
      track_events_(track_events),
      track_reason_(track_reason),
      debug_events_until_date_(debug_events_until_date),
      omit_details_(false) {}

std::uint64_t AllFlagsState::Metadata::Version() const {
    return version_;
}

std::optional<std::int64_t> AllFlagsState::Metadata::Variation() const {
    return variation_;
}

std::optional<EvaluationReason> const& AllFlagsState::Metadata::Reason() const {
    return reason_;
}

bool AllFlagsState::Metadata::TrackEvents() const {
    return track_events_;
}

bool AllFlagsState::Metadata::TrackReason() const {
    return track_reason_;
}

std::optional<std::uint64_t> const&
AllFlagsState::Metadata::DebugEventsUntilDate() const {
    return debug_events_until_date_;
}

bool AllFlagsState::Metadata::OmitDetails() const {
    return omit_details_;
}

AllFlagsState::AllFlagsState()
    : valid_(false), evaluations_(), flags_state_() {}

AllFlagsState::AllFlagsState(
    std::unordered_map<std::string, Value> evaluations,
    std::unordered_map<std::string, Metadata> flags_state)
    : valid_(true),
      evaluations_(std::move(evaluations)),
      flags_state_(std::move(flags_state)) {}

bool AllFlagsState::Valid() const {
    return valid_;
}

std::unordered_map<std::string, AllFlagsState::Metadata> const&
AllFlagsState::FlagsState() const {
    return flags_state_;
}

std::unordered_map<std::string, Value> const& AllFlagsState::Evaluations()
    const {
    return evaluations_;
}

bool operator==(AllFlagsState const& lhs, AllFlagsState const& rhs) {
    return lhs.Valid() == rhs.Valid() &&
           lhs.Evaluations() == rhs.Evaluations() &&
           lhs.FlagsState() == rhs.FlagsState();
}

bool operator==(AllFlagsState::Metadata const& lhs,
                AllFlagsState::Metadata const& rhs) {
    return lhs.Version() == rhs.Version() &&
           lhs.Variation() == rhs.Variation() && lhs.Reason() == rhs.Reason() &&
           lhs.TrackEvents() == rhs.TrackEvents() &&
           lhs.TrackReason() == rhs.TrackReason() &&
           lhs.DebugEventsUntilDate() == rhs.DebugEventsUntilDate() &&
           lhs.OmitDetails() == rhs.OmitDetails();
}

}  // namespace launchdarkly::server_side
