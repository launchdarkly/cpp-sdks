#include "all_flags_state_builder.hpp"

#include <launchdarkly/data_model/flag.hpp>

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

bool IsExperimentationEnabled(data_model::Flag const& flag,
                              std::optional<EvaluationReason> const& reason) {
    if (!reason) {
        return false;
    }
    if (reason->InExperiment()) {
        return true;
    }
    switch (reason->Kind()) {
        case EvaluationReason::Kind::kFallthrough:
            return flag.trackEventsFallthrough;
        case EvaluationReason::Kind::kRuleMatch:
            if (!reason->RuleIndex() ||
                reason->RuleIndex() >= flag.rules.size()) {
                return false;
            }
            return flag.rules.at(*reason->RuleIndex()).trackEvents;
        default:
            return false;
    }
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
