#pragma once

#include "launchdarkly/client_side/event_processor.hpp"

namespace launchdarkly::client_side::detail {

class NullEventProcessor : public IEventProcessor {
   public:
    NullEventProcessor() = default;
    void AsyncSend(events::InputEvent event) override;
    void AsyncFlush() override;
    void AsyncClose() override;
};
}  // namespace launchdarkly::client_side::detail
