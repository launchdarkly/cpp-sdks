#include <launchdarkly/serialization/json_evaluation_reason.hpp>
#include <launchdarkly/serialization/json_value.hpp>
#include <launchdarkly/server_side/serialization/json_all_flags_state.hpp>

#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>
#include "launchdarkly/serialization/value_mapping.hpp"

namespace launchdarkly::server_side {

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                server_side::AllFlagsState::State const& state) {
    boost::ignore_unused(unused);
    auto& obj = json_value.emplace_object();

    if (!state.OmitDetails()) {
        obj.emplace("version", state.Version());
        WriteMinimal(obj, "reason", state.Reason());
    }

    if (auto const& variation = state.Variation()) {
        obj.emplace("variation", *variation);
    }

    WriteMinimal(obj, "trackEvents", state.TrackEvents());
    WriteMinimal(obj, "trackReason", state.TrackReason());

    if (auto const& date = state.DebugEventsUntilDate()) {
        if (*date > 0) {
            obj.emplace("debugEventsUntilDate", boost::json::value_from(*date));
        }
    }

    if (auto const& prerequisites = state.Prerequisites();
        !prerequisites.empty()) {
        obj.emplace("prerequisites", boost::json::value_from(prerequisites));
    }
}

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                server_side::AllFlagsState const& state) {
    boost::ignore_unused(unused);
    auto& obj = json_value.emplace_object();
    obj.emplace("$valid", state.Valid());

    obj.emplace("$flagsState", boost::json::value_from(state.States()));

    for (auto const& [k, v] : state.Values()) {
        obj.emplace(k, boost::json::value_from(v));
    }
}
}  // namespace launchdarkly::server_side
