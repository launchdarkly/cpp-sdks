#pragma once

#include <launchdarkly/events/data/events.hpp>

#include <functional>

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
     * Asynchronously shutdown the processor. The processor attempts to
     * deliver all pending events, then invokes the completion.
     * @param completion Invoked when the delivery ends. It may be empty.
     */
    virtual void ShutdownAsync(std::function<void()> completion) = 0;

    virtual ~IEventProcessor() = default;
    IEventProcessor(IEventProcessor const& item) = delete;
    IEventProcessor(IEventProcessor&& item) = delete;
    IEventProcessor& operator=(IEventProcessor const&) = delete;
    IEventProcessor& operator=(IEventProcessor&&) = delete;

   protected:
    IEventProcessor() = default;
};

}  // namespace launchdarkly::events
