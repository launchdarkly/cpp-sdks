#pragma once

#include <launchdarkly/events/event_processor_interface.hpp>

#include "event_factory.hpp"

namespace launchdarkly::server_side {

class EventScope {
   public:
    EventScope(events::IEventProcessor& processor, EventFactory factory)
        : processor_(processor), factory_(std::move(factory)) {}

    template <typename Callable>
    void Get(Callable&& callable) const {
        processor_.SendAsync(callable(factory_));
    }

   private:
    events::IEventProcessor& processor_;
    EventFactory const factory_;
};

}  // namespace launchdarkly::server_side
