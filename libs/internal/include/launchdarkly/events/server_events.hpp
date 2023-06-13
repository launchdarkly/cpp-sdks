#pragma once

#include "common_events.hpp"

namespace launchdarkly::events::server {

struct IndexEvent {
    Date creation_date;
    EventContext context;
};

}  // namespace launchdarkly::events::server
