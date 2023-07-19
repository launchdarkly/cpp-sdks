#pragma once

#include <launchdarkly/events/event_processor_interface.hpp>

namespace launchdarkly::events {

class NullEventProcessor : public IEventProcessor {
   public:
    NullEventProcessor() = default;
    void SendAsync(events::InputEvent event) override;
    void FlushAsync() override;
    void ShutdownAsync() override;
};
}  // namespace launchdarkly::events
