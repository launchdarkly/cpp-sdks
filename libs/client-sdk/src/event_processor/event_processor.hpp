#pragma once

#include <boost/asio/any_io_executor.hpp>

#include <launchdarkly/config/client.hpp>
#include <launchdarkly/config/sdks.hpp>
#include <launchdarkly/events/asio_event_processor.hpp>
#include <launchdarkly/logging/logger.hpp>
#include "../event_processor.hpp"

namespace launchdarkly::client_side {

class EventProcessor : public IEventProcessor {
   public:
    EventProcessor(boost::asio::any_io_executor const& io,
                   Config const& config,
                   Logger& logger);
    void AsyncSend(events::InputEvent event) override;
    void AsyncFlush() override;
    void AsyncClose() override;

   private:
    events::AsioEventProcessor<SDK> impl_;
};

}  // namespace launchdarkly::client_side
