#pragma once

#include "event_factory.hpp"

namespace launchdarkly::server_side {

struct EventScope {
    bool const disabled;
    EventFactory factory;
};

}  // namespace launchdarkly::server_side
