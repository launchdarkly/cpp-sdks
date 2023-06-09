#include "null_event_processor.hpp"

namespace launchdarkly::client_side {

void NullEventProcessor::SendAsync(events::InputEvent event) {}

void NullEventProcessor::FlushAsync() {}

void NullEventProcessor::ShutdownAsync() {}
}  // namespace launchdarkly::client_side
