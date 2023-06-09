#pragma once

#include "../event_processor.hpp"

namespace launchdarkly::client_side {

class NullEventProcessor : public IEventProcessor {
   public:
    NullEventProcessor() = default;
    void SendAsync(events::InputEvent event) override;
    void FlushAsync() override;
    void ShutdownAsync() override;
};
}  // namespace launchdarkly::client_side
