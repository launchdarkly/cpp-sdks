#include "event_factory.hpp"

#include <chrono>
namespace launchdarkly::server_side {

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

events::Date Now() {
    return events::Date{std::chrono::system_clock::now()};
}

EventFactory::EventFactory(
    launchdarkly::server_side::EventFactory::ReasonPolicy reason_policy)
    : reason_policy_(reason_policy) {}

EventFactory EventFactory::WithReasons() {
    return {ReasonPolicy::Send};
}

EventFactory EventFactory::WithoutReasons() {
    return {ReasonPolicy::Default};
}

events::InputEvent EventFactory::UnknownFlag(
    std::string const& key,
    launchdarkly::Context const& ctx,
    EvaluationDetail<launchdarkly::Value> detail,
    launchdarkly::Value default_val) {
    return FeatureRequest(key, ctx, std::nullopt, detail, default_val,
                          std::nullopt);
}

events::InputEvent EventFactory::Eval(
    std::string const& key,
    Context const& ctx,
    std::optional<data_model::Flag> const& flag,
    EvaluationDetail<Value> detail,
    Value default_value,
    std::optional<std::string> prereq_of) {
    return FeatureRequest(key, ctx, flag, detail, default_value, prereq_of);
}

events::InputEvent EventFactory::Identify(launchdarkly::Context ctx) {
    return events::IdentifyEventParams{Now(), std::move(ctx)};
}

events::InputEvent EventFactory::FeatureRequest(
    std::string const& key,
    launchdarkly::Context const& context,
    std::optional<data_model::Flag> const& flag,
    EvaluationDetail<launchdarkly::Value> detail,
    launchdarkly::Value default_val,
    std::optional<std::string> prereq_of) {
    bool flag_track_events = false;
    bool require_experiment_data = false;
    std::optional<events::Date> debug_events_until_date;

    if (flag.has_value()) {
        flag_track_events = flag->trackEvents;
        require_experiment_data =
            IsExperimentationEnabled(*flag, detail.Reason());
        if (flag->debugEventsUntilDate) {
            debug_events_until_date =
                events::Date{std::chrono::system_clock::time_point{
                    std::chrono::milliseconds(*flag->debugEventsUntilDate)}};
        }
    } else {
        flag_track_events = false;
        require_experiment_data = false;
    }

    std::optional<launchdarkly::EvaluationReason> reason;
    if (reason_policy_ == ReasonPolicy::Send || require_experiment_data) {
        reason = detail.Reason();
    }

    return events::FeatureEventParams{
        events::Date{std::chrono::system_clock::now()},
        key,
        context,
        detail.Value(),
        default_val,
        flag.has_value() ? std::make_optional(flag->version) : std::nullopt,
        detail.VariationIndex(),
        reason,
        flag_track_events || require_experiment_data,
        debug_events_until_date};
}

}  // namespace launchdarkly::server_side
