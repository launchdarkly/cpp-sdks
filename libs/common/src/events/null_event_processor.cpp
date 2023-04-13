#include "events/detail/null_event_processor.hpp"

namespace launchdarkly::events::detail {
void NullEventProcessor::AsyncSend(InputEvent event) {}

void NullEventProcessor::AsyncFlush() {}

void NullEventProcessor::AsyncClose() {}
}  // namespace launchdarkly::events::detail
