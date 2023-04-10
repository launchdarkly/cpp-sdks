#pragma once

#include "events/client_events.hpp"
// #include "events/server_events.hpp"
// Server-side events would be added to the Input/Output event variants.

namespace launchdarkly::events {

using InputEvent = std::variant<client::FeatureEventParams,
                                client::IdentifyEventParams,
                                TrackEventParams>;

using OutputEvent = std::variant<client::FeatureEvent,
                                 client::DebugEvent,
                                 client::IdentifyEvent,
                                 TrackEvent>;

}  // namespace launchdarkly::events
