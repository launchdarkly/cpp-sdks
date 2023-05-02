#include "launchdarkly/client_side/event_processor/detail/null_event_processor.hpp"

namespace launchdarkly::client_side::detail {

void NullEventProcessor::AsyncSend(events::InputEvent event) {}

void NullEventProcessor::AsyncFlush() {}

void NullEventProcessor::AsyncClose() {}
}  // namespace launchdarkly::client_side::detail
