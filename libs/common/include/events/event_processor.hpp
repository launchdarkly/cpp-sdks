#pragma once

#include "events/events.hpp"

namespace launchdarkly::events {

class IEventProcessor {
   public:
    virtual ~IEventProcessor() = default;
    /**
     * Asynchronously delivers an event to the processor, returning as
     * soon as possible. The event may be dropped if the processor lacks
     * capacity.
     * @param event InputEvent to deliver.
     */
    virtual void AsyncSend(InputEvent event) = 0;
    /**
     * Asynchronously flush's the processor's events, returning as soon as
     * possible. Flushing may be a no-op if a flush is ongoing.
     */
    virtual void AsyncFlush() = 0;
    /**
     * Asynchronously shutdown the processor. The processor should attempt to
     * flush all events before shutting down.
     */
    virtual void AsyncClose() = 0;
};

}  // namespace launchdarkly::events
