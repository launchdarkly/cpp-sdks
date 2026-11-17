#include <launchdarkly/events/null_event_processor.hpp>

namespace launchdarkly::events {

void NullEventProcessor::SendAsync(events::InputEvent event) {}

void NullEventProcessor::FlushAsync() {}

void NullEventProcessor::ShutdownAsync(std::function<void()> completion) {
    if (completion) {
        completion();
    }
}
}  // namespace launchdarkly::events
