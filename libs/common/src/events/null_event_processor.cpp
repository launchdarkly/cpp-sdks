#include "events/detail/null_event_processor.hpp"

namespace launchdarkly::events::detail {
void NullEventProcessor::async_send(InputEvent event) {}

void NullEventProcessor::async_flush() {}

void NullEventProcessor::async_close() {}
}  // namespace launchdarkly::events::detail
