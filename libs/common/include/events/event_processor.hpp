#pragma once

#include "events/events.hpp"

namespace launchdarkly::events {

class IEventProcessor {
   public:
    virtual ~IEventProcessor() = default;
    /**
     * Asynchronously delivers an event to the processor's inbox, returning as
     * soon as possible. The event may be dropped if the inbox lacks capacity.
     * @param event InputEvent to deliver.
     */
    virtual void async_send(InputEvent event) = 0;
    /**
     * Asynchronously flush the processor's outbox, returning as soon as possible.
     * Flushing may be a no-op if a flush is already in progress.
     */
    virtual void async_flush() = 0;
    /**
     * Synchronously close the processor. The processor should attempt to flush
     * all events in the outbox before shutting down. All asynchronous operations
     * MUST be completed before sync_close returns.
     */
    virtual void sync_close() = 0;
};

}  // namespace launchdarkly::events
