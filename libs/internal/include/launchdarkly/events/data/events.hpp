#pragma once

#include <launchdarkly/events/data/common_events.hpp>
#include <launchdarkly/events/data/server_events.hpp>

#include <variant>
#include "common_events.hpp"

namespace launchdarkly::events {

using InputEvent =
    std::variant<FeatureEventParams, IdentifyEventParams, TrackEventParams>;

using OutputEvent = std::variant<FeatureEvent,
                                 DebugEvent,
                                 IdentifyEvent,
                                 server_side::IndexEvent,
                                 TrackEvent>;

}  // namespace launchdarkly::events
