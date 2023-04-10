#pragma once
#include <atomic>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/steady_timer.hpp>
#include <queue>
#include <variant>
#include "config/detail/events.hpp"
#include "config/detail/service_hosts.hpp"
#include "events/detail/dispatcher.hpp"
#include "events/event_processor.hpp"
#include "events/events.hpp"
#include "logger.hpp"

namespace launchdarkly::events::detail {

class AsioEventProcessor : public IEventProcessor {
   public:
    AsioEventProcessor(boost::asio::any_io_executor const& executor,
                       config::detail::Events const& config,
                       config::ServiceHosts const& endpoints,
                       Logger& logger);

    void AsyncSend(InputEvent event) override;
    void AsyncFlush() override;
    void AsyncClose() override;

   private:
    Logger& logger_;
    Dispatcher dispatcher_;
};
}  // namespace launchdarkly::events::detail
