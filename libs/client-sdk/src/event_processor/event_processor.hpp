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
                   config::shared::built::ServiceEndpoints const& endpoints,
                   config::shared::built::Events const& events_config,
                   config::shared::built::HttpProperties const& http_properties,
                   Logger& logger);
    void SendAsync(events::InputEvent event) override;
    void FlushAsync() override;
    void ShutdownAsync() override;

   private:
    events::AsioEventProcessor<SDK> impl_;
};

}  // namespace launchdarkly::client_side
