#pragma once

#include "client_events.hpp"
#include "server_events.hpp"

namespace launchdarkly::events {

using InputEvent = std::variant<client::FeatureEventParams,
                                client::IdentifyEventParams,
                                TrackEventParams>;

using OutputEvent = std::variant<client::FeatureEvent,
                                 client::DebugEvent,
                                 client::IdentifyEvent,
                                 server::IndexEvent,
                                 TrackEvent>;

}  // namespace launchdarkly::events
