#include "null_event_processor.hpp"

namespace launchdarkly::client_side {

void NullEventProcessor::AsyncSend(events::InputEvent event) {}

void NullEventProcessor::AsyncFlush() {}

void NullEventProcessor::AsyncClose() {}
}  // namespace launchdarkly::client_side
