#include "event_factory.hpp"

#include <chrono>
namespace launchdarkly::server_side {

EventFactory::EventFactory(
    launchdarkly::server_side::EventFactory::ReasonPolicy reason_policy)
    : reason_policy_(reason_policy),
      now_([]() { return events::Date{std::chrono::system_clock::now()}; }) {}

EventFactory EventFactory::WithReasons() {
    return {ReasonPolicy::Require};
}

EventFactory EventFactory::WithoutReasons() {
    return {ReasonPolicy::Default};
}

events::InputEvent EventFactory::UnknownFlag(
    std::string const& key,
    launchdarkly::Context const& ctx,
    EvaluationDetail<launchdarkly::Value> detail,
    launchdarkly::Value default_val) const {
    return FeatureRequest(key, ctx, std::nullopt, detail, default_val,
                          std::nullopt);
}

events::InputEvent EventFactory::Eval(
    std::string const& key,
    Context const& ctx,
    std::optional<data_model::Flag> const& flag,
    EvaluationDetail<Value> detail,
    Value default_value,
    std::optional<std::string> prereq_of) const {
    return FeatureRequest(key, ctx, flag, detail, default_value, prereq_of);
}

events::InputEvent EventFactory::Identify(launchdarkly::Context ctx) const {
    return events::IdentifyEventParams{now_(), std::move(ctx)};
}

events::InputEvent EventFactory::Custom(
    Context const& ctx,
    std::string event_name,
    std::optional<Value> data,
    std::optional<double> metric_value) const {
    return events::ServerTrackEventParams{
        {now_(), std::move(event_name), ctx.KindsToKeys(), std::move(data),
         metric_value},
        ctx};
}

events::InputEvent EventFactory::FeatureRequest(
    std::string const& key,
    launchdarkly::Context const& context,
    std::optional<data_model::Flag> const& flag,
    EvaluationDetail<launchdarkly::Value> detail,
    launchdarkly::Value default_val,
    std::optional<std::string> prereq_of) const {
    bool flag_track_events = false;
    bool require_experiment_data = false;
    std::optional<events::Date> debug_events_until_date;

    if (flag.has_value()) {
        flag_track_events = flag->trackEvents;
        require_experiment_data =
            flag->IsExperimentationEnabled(detail.Reason());
        if (flag->debugEventsUntilDate) {
            debug_events_until_date =
                events::Date{std::chrono::system_clock::time_point{
                    std::chrono::milliseconds(*flag->debugEventsUntilDate)}};
        }
    }

    std::optional<launchdarkly::EvaluationReason> reason;
    if (reason_policy_ == ReasonPolicy::Require || require_experiment_data) {
        reason = detail.Reason();
    }

    return events::FeatureEventParams{
        now_(),
        key,
        context,
        detail.Value(),
        default_val,
        flag.has_value() ? std::make_optional(flag->version) : std::nullopt,
        detail.VariationIndex(),
        reason,
        flag_track_events || require_experiment_data,
        debug_events_until_date,
        prereq_of};
}

}  // namespace launchdarkly::server_side
