#pragma once
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/steady_timer.hpp>
#include <queue>
#include <variant>
#include "config/detail/events.hpp"
#include "events/event_processor.hpp"
#include "events/events.hpp"
#include "logger.hpp"

namespace launchdarkly::events {

class AsioEventProcessor : public IEventProcessor {
   public:
    AsioEventProcessor(boost::asio::any_io_executor const& executor,
                       config::detail::Events config,
                       Logger& logger);

    void async_send(InputEvent event) override;
    void async_flush() override;
    void sync_close() override;

   private:
    struct FlushCommand {};
    struct CloseCommand {};
    struct SendCommand {};
    using Command = std::variant<FlushCommand, CloseCommand, SendCommand>;
    using Inbox = std::queue<Command>;

    Inbox inbox_;
    boost::asio::any_io_executor io_;
    boost::asio::steady_timer flush_timer_;
    Logger& logger_;
    bool shutdown_;

    void flush(boost::system::error_code ec);
    void do_flush();
};
}  // namespace launchdarkly::events
