#include "events/detail/asio_event_processor.hpp"

#include <boost/asio/strand.hpp>
#include <chrono>

namespace launchdarkly::events::detail {

AsioEventProcessor::AsioEventProcessor(
    boost::asio::any_io_executor const& executor,
    config::detail::Events const& config,
    config::ServiceHosts const& endpoints,
    Logger& logger)
    : logger_(logger),
      dispatcher_(boost::asio::make_strand(executor),
                  config,
                  endpoints,
                  "password",
                  logger) {}

void AsioEventProcessor::AsyncSend(InputEvent in_event) {
    LD_LOG(logger_, LogLevel::kDebug) << "processor: pushing event into inbox";
    dispatcher_.AsyncSend(std::move(in_event));
}

void AsioEventProcessor::AsyncFlush() {
    LD_LOG(logger_, LogLevel::kDebug)
        << "processor: requesting unscheduled flush";
    dispatcher_.AsyncFlush();
}

void AsioEventProcessor::AsyncClose() {
    LD_LOG(logger_, LogLevel::kDebug) << "processor: request shutdown";
    dispatcher_.AsyncFlush();
    dispatcher_.AsyncClose();
}

}  // namespace launchdarkly::events::detail
