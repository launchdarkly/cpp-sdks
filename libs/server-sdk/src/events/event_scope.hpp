#pragma once

#include <launchdarkly/events/event_processor_interface.hpp>

#include "event_factory.hpp"

namespace launchdarkly::server_side {

class EventScope {
   public:
    EventScope(bool enabled,
               events::IEventProcessor& processor,
               EventFactory factory)
        : enabled_(enabled),
          processor_(processor),
          factory_(std::move(factory)) {}

    template <typename Callable>
    void Get(Callable&& callable) const {
        if (enabled_) {
            callable(processor_, factory_);
        }
    }

   private:
    bool const enabled_;
    events::IEventProcessor& processor_;
    EventFactory const factory_;
};

}  // namespace launchdarkly::server_side
