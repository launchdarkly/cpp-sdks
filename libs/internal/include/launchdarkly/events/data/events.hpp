#pragma once

#include <launchdarkly/events/data/common_events.hpp>
#include <launchdarkly/events/data/server_events.hpp>

#include <variant>

namespace launchdarkly::events {

using InputEvent = std::variant<FeatureEventParams,
                                IdentifyEventParams,
                                ClientTrackEventParams,
                                ServerTrackEventParams>;

using OutputEvent = std::variant<FeatureEvent,
                                 DebugEvent,
                                 IdentifyEvent,
                                 server_side::IndexEvent,
                                 TrackEvent>;

}  // namespace launchdarkly::events
