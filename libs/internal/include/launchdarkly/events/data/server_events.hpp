#pragma once

#include <launchdarkly/events/data/common_events.hpp>

namespace launchdarkly::events::server_side {

struct IndexEvent {
    Date creation_date;
    EventContext context;
};

}  // namespace launchdarkly::events::server_side
