#include "launchdarkly/client_side/event_processor/detail/event_processor.hpp"

namespace launchdarkly::client_side::detail {

EventProcessor::EventProcessor(boost::asio::any_io_executor io,
                               Config const& config,
                               Logger& logger)
    : impl_(io, config, logger) {}

void EventProcessor::AsyncSend(launchdarkly::events::InputEvent event) {
    impl_.AsyncSend(std::move(event));
}

void EventProcessor::AsyncFlush() {
    impl_.AsyncFlush();
}

void EventProcessor::AsyncClose() {
    impl_.AsyncClose();
}

}  // namespace launchdarkly::client_side::detail
