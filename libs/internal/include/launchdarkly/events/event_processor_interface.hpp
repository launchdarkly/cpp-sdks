#pragma once

#include <launchdarkly/events/data/events.hpp>

namespace launchdarkly::events {

class IEventProcessor {
   public:
    /**
     * Asynchronously delivers an event to the processor, returning as
     * soon as possible. The event may be dropped if the processor lacks
     * capacity.
     * @param event InputEvent to deliver.
     */
    virtual void SendAsync(InputEvent event) = 0;
    /**
     * Asynchronously flush's the processor's events, returning as soon as
     * possible. Flushing may be a no-op if a flush is ongoing.
     */
    virtual void FlushAsync() = 0;
    /**
     * Asynchronously shutdown the processor. The processor should attempt to
     * flush all events before shutting down.
     */
    virtual void ShutdownAsync() = 0;

    virtual ~IEventProcessor() = default;
    IEventProcessor(IEventProcessor const& item) = delete;
    IEventProcessor(IEventProcessor&& item) = delete;
    IEventProcessor& operator=(IEventProcessor const&) = delete;
    IEventProcessor& operator=(IEventProcessor&&) = delete;

   protected:
    IEventProcessor() = default;
};

}  // namespace launchdarkly::events
