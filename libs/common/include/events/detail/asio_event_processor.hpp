#pragma once
#include <atomic>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/steady_timer.hpp>
#include <queue>
#include <variant>
#include "config/detail/events.hpp"
#include "events/detail/dispatcher.hpp"
#include "events/event_processor.hpp"
#include "events/events.hpp"
#include "logger.hpp"

namespace launchdarkly::events::detail {

class AsioEventProcessor : public IEventProcessor {
   public:
    AsioEventProcessor(boost::asio::any_io_executor const& executor,
                       config::detail::Events config,
                       Logger& logger);

    ~AsioEventProcessor();

    void async_send(InputEvent event) override;
    void async_flush() override;
    void async_close() override;

   private:
    Logger& logger_;
    Dispatcher dispatcher_;
};
}  // namespace launchdarkly::events::detail
