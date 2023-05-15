#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <launchdarkly/client_side/event_processor.hpp>
#include <launchdarkly/config/client.hpp>
#include <launchdarkly/events/detail/asio_event_processor.hpp>
#include "launchdarkly/config/sdks.hpp"
#include "launchdarkly/logging/logger.hpp"

namespace launchdarkly::client_side::detail {

class EventProcessor : public IEventProcessor {
   public:
    EventProcessor(boost::asio::any_io_executor const& io,
                   Config const& config,
                   Logger& logger);
    void AsyncSend(events::InputEvent event) override;
    void AsyncFlush() override;
    void AsyncClose() override;

   private:
    events::detail::AsioEventProcessor<SDK> impl_;
};

}  // namespace launchdarkly::client_side::detail
