#include "event_processor.hpp"

namespace launchdarkly::client_side {

EventProcessor::EventProcessor(
    boost::asio::any_io_executor const& io,
    std::string sdk_key,
    config::shared::built::ServiceEndpoints endpoints,
    config::shared::built::Events events_config,
    config::shared::built::HttpProperties http_properties,
    Logger& logger)
    : impl_(io, sdk_key, endpoints, events_config, http_properties, logger) {}

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
