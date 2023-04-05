#include "events/detail/null_event_processor.hpp"

namespace launchdarkly::events {
void NullEventProcessor::async_send(InputEvent event) {}

void NullEventProcessor::async_flush() {}

void NullEventProcessor::sync_close() {}
}  // namespace launchdarkly::events
