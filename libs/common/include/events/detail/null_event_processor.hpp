#pragma once

#include "events/event_processor.hpp"

namespace launchdarkly::events {

class NullEventProcessor : public IEventProcessor {
   public:
    NullEventProcessor() = default;
    void async_send(InputEvent event) override;
    void async_flush() override;
    void sync_close() override;
};
}  // namespace launchdarkly::events
