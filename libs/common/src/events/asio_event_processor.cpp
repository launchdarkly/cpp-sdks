#include "events/detail/asio_event_processor.hpp"

#include <boost/asio/strand.hpp>
#include <chrono>

namespace launchdarkly::events::detail {

AsioEventProcessor::AsioEventProcessor(
    boost::asio::any_io_executor const& executor,
    config::detail::Events const& config,
    config::ServiceEndpoints const& endpoints,
    Logger& logger)
    : logger_(logger),
      dispatcher_(boost::asio::make_strand(executor),
                  config,
                  endpoints,
                  "password",
                  logger) {}

void AsioEventProcessor::async_send(InputEvent in_event) {
    LD_LOG(logger_, LogLevel::kDebug) << "processor: pushing event into inbox";
    dispatcher_.send(std::move(in_event));
}

void AsioEventProcessor::async_flush() {
    LD_LOG(logger_, LogLevel::kDebug)
        << "processor: requesting unscheduled flush";
    dispatcher_.request_flush();
}

void AsioEventProcessor::async_close() {
    LD_LOG(logger_, LogLevel::kDebug) << "processor: request shutdown";
    dispatcher_.request_flush();
    dispatcher_.shutdown();
}

}  // namespace launchdarkly::events::detail
