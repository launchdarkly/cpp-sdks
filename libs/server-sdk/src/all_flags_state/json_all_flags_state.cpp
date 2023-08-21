#include "launchdarkly/serialization/json_evaluation_reason.hpp"
#include "launchdarkly/serialization/json_primitives.hpp"
#include "launchdarkly/serialization/json_value.hpp"
#include "launchdarkly/server_side/json_feature_flags_state.hpp"

#include <boost/core/ignore_unused.hpp>

namespace launchdarkly::server_side {

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                server_side::AllFlagsState::Metadata const& state) {
    boost::ignore_unused(unused);
    auto& obj = json_value.emplace_object();

    if (!state.OmitDetails()) {
        obj.emplace("version", state.Version());

        if (auto const& reason = state.Reason()) {
            obj.emplace("reason", boost::json::value_from(*reason));
        }
    }

    if (auto const& variation = state.Variation()) {
        obj.emplace("variation", *variation);
    }

    if (state.TrackEvents()) {
        obj.emplace("trackEvents", true);
    }

    if (state.TrackReason()) {
        obj.emplace("trackReason", true);
    }

    if (auto const& date = state.DebugEventsUntilDate()) {
        if (*date > 0) {
            obj.emplace("debugEventsUntilDate", boost::json::value_from(*date));
        }
    }
}

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                server_side::AllFlagsState const& state) {
    boost::ignore_unused(unused);
    auto& obj = json_value.emplace_object();
    obj.emplace("$valid", state.Valid());

    obj.emplace("$flagsState", boost::json::value_from(state.FlagsState()));

    for (auto const& [k, v] : state.Evaluations()) {
        obj.emplace(k, boost::json::value_from(v));
    }
}
}  // namespace launchdarkly::server_side
