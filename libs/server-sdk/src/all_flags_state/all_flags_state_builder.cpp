#include "all_flags_state_builder.hpp"

namespace launchdarkly::server_side {

bool IsExperimentationEnabled(data_model::Flag const& flag,
                              std::optional<EvaluationReason> const& reason);
bool IsSet(AllFlagsStateOptions options, AllFlagsStateOptions flag);

AllFlagsStateOptions operator&(AllFlagsStateOptions lhs,
                               AllFlagsStateOptions rhs);

AllFlagsStateBuilder::AllFlagsStateBuilder(evaluation::Evaluator& evaluator,
                                           data_store::IDataStore const& store)
    : evaluator_(evaluator), store_(store) {}

FeatureFlagsState AllFlagsStateBuilder::Build(
    launchdarkly::Context const& context,
    enum AllFlagsStateOptions options) {
    std::unordered_map<std::string, FeatureFlagsState::FlagState> flags_state;
    std::unordered_map<std::string, Value> evaluations;

    for (auto [key, flag_desc] : store_.AllFlags()) {
        if (!flag_desc || !flag_desc->item) {
            continue;
        }
        auto const& flag = (*flag_desc->item);
        if (IsSet(options, AllFlagsStateOptions::ClientSideOnly) &&
            !flag.clientSideAvailability.usingEnvironmentId) {
            continue;
        }

        auto detail = evaluator_.Evaluate(flag, context);

        bool require_experiment_data =
            IsExperimentationEnabled(flag, detail.Reason());
        bool track_events = flag.trackEvents || require_experiment_data;
        bool track_reason = require_experiment_data;
        bool currently_debugging = false;
        if (flag.debugEventsUntilDate) {
            auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
            currently_debugging = *flag.debugEventsUntilDate > now;
        }

        bool omit_details =
            (IsSet(options, AllFlagsStateOptions::DetailsOnlyForTrackedFlags) &&
             !(track_events || track_reason || currently_debugging));

        std::optional<EvaluationReason> reason =
            (!IsSet(options, AllFlagsStateOptions::IncludeReasons) &&
             !track_reason)
                ? std::nullopt
                : detail.Reason();

        if (omit_details) {
            reason = std::nullopt;
            /* version = std::nullopt; */
        }

        evaluations.emplace(key, detail.Value());
        flags_state.emplace(
            key, FeatureFlagsState::FlagState{
                     flag.version, detail.VariationIndex(), reason,
                     track_events, track_reason, flag.debugEventsUntilDate});
    }

    return FeatureFlagsState{std::move(evaluations), std::move(flags_state)};
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

AllFlagsStateOptions operator&(AllFlagsStateOptions lhs,
                               AllFlagsStateOptions rhs) {
    return static_cast<AllFlagsStateOptions>(
        static_cast<std::underlying_type_t<AllFlagsStateOptions>>(lhs) &
        static_cast<std::underlying_type_t<AllFlagsStateOptions>>(rhs));
}

bool IsSet(AllFlagsStateOptions options, AllFlagsStateOptions flag) {
    return (options & flag) == flag;
}

}  // namespace launchdarkly::server_side
