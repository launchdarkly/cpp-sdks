#pragma once

#include <launchdarkly/server_side/all_flags_state.hpp>

#include <boost/json/fwd.hpp>

namespace launchdarkly::server_side {

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                server_side::AllFlagsState::State const& state);

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                server_side::AllFlagsState const& state);
}  // namespace launchdarkly::server_side
