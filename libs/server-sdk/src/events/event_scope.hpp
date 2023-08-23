#pragma once

#include <launchdarkly/events/event_processor_interface.hpp>

#include "event_factory.hpp"

namespace launchdarkly::server_side {

/**
 * EventScope is responsible for forwarding events, created by a factory, to an
 * event processor interface. If the interface is nullptr, then events will not
 * be forwarded at all.
 */
class EventScope {
   public:
    /**
     * Constructs an EventScope with a non-owned IEventProcessor and factory.
     * When Get is called, the factory will be passed to the caller who may then
     * select a particular event to construct.
     * @param processor The event processor to forward events to.
     * @param factory The factory used for generating events.
     */
    EventScope(events::IEventProcessor* processor, EventFactory factory)
        : processor_(processor), factory_(std::move(factory)) {}

    /**
     * Default constructs an EventScope which will not forward events.
     */
    EventScope() : EventScope(nullptr, EventFactory::WithoutReasons()) {}

    template <typename Callable>
    void Get(Callable&& callable) const {
        if (processor_) {
            processor_->SendAsync(callable(factory_));
        }
    }

   private:
    events::IEventProcessor* processor_;
    EventFactory const factory_;
};

}  // namespace launchdarkly::server_side
