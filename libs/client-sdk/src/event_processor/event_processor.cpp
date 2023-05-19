#include "event_processor.hpp"

namespace launchdarkly::client_side {

EventProcessor::EventProcessor(
    boost::asio::any_io_executor const& io,
    config::shared::built::ServiceEndpoints const& endpoints,
    config::shared::built::Events const& events_config,
    config::shared::built::HttpProperties const& http_properties,
    Logger& logger)
    : impl_(io, endpoints, events_config, http_properties, logger) {}

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
