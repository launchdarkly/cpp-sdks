#include <launchdarkly/serialization/json_evaluation_reason.hpp>
#include <launchdarkly/serialization/json_primitives.hpp>
#include <launchdarkly/serialization/json_value.hpp>
#include <launchdarkly/server_side/json_feature_flags_state.hpp>

#include <boost/core/ignore_unused.hpp>

namespace launchdarkly::server_side {

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                server_side::FeatureFlagsState::FlagState const& state) {
    boost::ignore_unused(unused);
    auto& obj = json_value.emplace_object();
    if (state.version) {
        obj.emplace("version", *state.version);
    }
    if (state.variation) {
        obj.emplace("variation", *state.variation);
    }
    if (state.reason) {
        obj.emplace("reason", boost::json::value_from(*state.reason));
    }
    if (state.track_events) {
        obj.emplace("trackEvents", true);
    }
    if (state.track_reason) {
        obj.emplace("trackReason", true);
    }
    if (state.debug_events_until_date) {
        obj.emplace("debugEventsUntilDate",
                    boost::json::value_from(*state.debug_events_until_date));
    }
}

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                server_side::FeatureFlagsState const& state) {
    boost::ignore_unused(unused);
    auto& obj = json_value.emplace_object();
    obj.emplace("$valid", state.Valid());

    obj.emplace("$flagsState", boost::json::value_from(state.FlagsState()));

    for (auto const [k, v] : state.Evaluations()) {
        obj.emplace(k, boost::json::value_from(v));
    }
}
}  // namespace launchdarkly::server_side
