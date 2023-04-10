#pragma once

#include "events/client_events.hpp"
namespace launchdarkly::events {

using InputEvent = std::variant<client::FeatureEventParams,
                                client::IdentifyEventParams,
                                TrackEventParams>;

using OutputEvent = std::variant<client::FeatureEvent,
                                 client::DebugEvent,
                                 client::IdentifyEvent,
                                 TrackEvent>;

}  // namespace launchdarkly::events
