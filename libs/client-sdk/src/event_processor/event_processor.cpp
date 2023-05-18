#include "event_processor.hpp"

namespace launchdarkly::client_side {

EventProcessor::EventProcessor(boost::asio::any_io_executor const& io,
                               Config const& config,
                               Logger& logger)
    : impl_(io, config, logger) {}

void EventProcessor::SendAsync(events::InputEvent event) {
    impl_.AsyncSend(std::move(event));
}

void EventProcessor::FlushAsync() {
    impl_.AsyncFlush();
}

void EventProcessor::ShutdownAsync() {
    impl_.AsyncClose();
}

}  // namespace launchdarkly::client_side
