#include "all_flags_state_builder.hpp"

namespace launchdarkly::server_side {

bool IsDebuggingEnabled(std::optional<std::uint64_t> debug_events_until);

AllFlagsStateBuilder::AllFlagsStateBuilder(AllFlagsState::Options options)
    : options_(options), flags_state_(), evaluations_() {}

void AllFlagsStateBuilder::AddFlag(std::string const& key,
                                   Value value,
                                   AllFlagsState::State flag) {
    if (IsSet(options_, AllFlagsState::Options::DetailsOnlyForTrackedFlags)) {
        if (!flag.TrackEvents() && !flag.TrackReason() &&
            !IsDebuggingEnabled(flag.DebugEventsUntilDate())) {
            flag.omit_details_ = true;
        }
    }
    if (NotSet(options_, AllFlagsState::Options::IncludeReasons) &&
        !flag.TrackReason()) {
        flag.reason_ = std::nullopt;
    }
    flags_state_.emplace(key, std::move(flag));
    evaluations_.emplace(std::move(key), std::move(value));
}

AllFlagsState AllFlagsStateBuilder::Build() {
    return AllFlagsState{std::move(evaluations_), std::move(flags_state_)};
}

bool IsSet(AllFlagsState::Options options, AllFlagsState::Options flag) {
    return (options & flag) == flag;
}

bool NotSet(AllFlagsState::Options options, AllFlagsState::Options flag) {
    return !IsSet(options, flag);
}

std::uint64_t NowUnixMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

bool IsDebuggingEnabled(std::optional<std::uint64_t> debug_events_until) {
    return debug_events_until && *debug_events_until > NowUnixMillis();
}

}  // namespace launchdarkly::server_side
