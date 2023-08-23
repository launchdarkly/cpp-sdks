#pragma once

#include "event_factory.hpp"

namespace launchdarkly::server_side {

class EventScope {
   public:
    EventScope(bool disabled, EventFactory factory)
        : disabled_(disabled), factory_(std::move(factory)) {}

    template <typename Callable>
    void Get(Callable&& callable) const {
        if (!disabled_) {
            callable(factory_);
        }
    }

   private:
    bool const disabled_;
    EventFactory const factory_;
};

}  // namespace launchdarkly::server_side
