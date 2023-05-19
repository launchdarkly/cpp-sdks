#pragma once

#include <boost/asio/any_io_executor.hpp>

#include <launchdarkly/config/client.hpp>
#include <launchdarkly/config/shared/sdks.hpp>
#include <launchdarkly/events/asio_event_processor.hpp>
#include <launchdarkly/logging/logger.hpp>

#include "../event_processor.hpp"

namespace launchdarkly::client_side {

class EventProcessor : public IEventProcessor {
   public:
    EventProcessor(boost::asio::any_io_executor const& io,
                   std::string sdk_key,
                   config::shared::built::ServiceEndpoints endpoints,
                   config::shared::built::Events events_config,
                   config::shared::built::HttpProperties http_properties,
                   Logger& logger);
    void SendAsync(events::InputEvent event) override;
    void FlushAsync() override;
    void ShutdownAsync() override;

   private:
    events::AsioEventProcessor<SDK> impl_;
};

}  // namespace launchdarkly::client_side
