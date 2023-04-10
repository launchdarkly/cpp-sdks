#pragma once

#include "events/event_processor.hpp"

namespace launchdarkly::events::detail {

class NullEventProcessor : public IEventProcessor {
   public:
    NullEventProcessor() = default;
    void AsyncSend(InputEvent event) override;
    void AsyncFlush() override;
    void AsyncClose() override;
};
}  // namespace launchdarkly::events::detail
