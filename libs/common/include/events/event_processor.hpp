#pragma once

#include "events/events.hpp"

namespace launchdarkly::events {

class IEventProcessor {
   public:
    virtual ~IEventProcessor() = default;
    virtual void async_send(InputEvent event) = 0;
    virtual void async_flush() = 0;
    virtual void async_close() = 0;
};

}  // namespace launchdarkly::events
