#pragma once

#include "../event_processor.hpp"

namespace launchdarkly::client_side {

class NullEventProcessor : public IEventProcessor {
   public:
    NullEventProcessor() = default;
    void AsyncSend(events::InputEvent event) override;
    void AsyncFlush() override;
    void AsyncClose() override;
};
}  // namespace launchdarkly::client_side
