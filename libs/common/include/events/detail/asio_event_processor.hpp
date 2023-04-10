#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/http.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <chrono>
#include <optional>
#include "config/detail/events.hpp"
#include "config/detail/service_hosts.hpp"
#include "context_filter.hpp"
#include "events/detail/conn_pool.hpp"
#include "events/detail/outbox.hpp"
#include "events/detail/summary_state.hpp"
#include "events/event_processor.hpp"
#include "events/events.hpp"
#include "logger.hpp"

namespace launchdarkly::events::detail {

class AsioEventProcessor : public IEventProcessor {
   public:
    AsioEventProcessor(boost::asio::any_io_executor io,
                       config::detail::Events const& config,
                       config::ServiceHosts const& endpoints,
                       std::string authorization,
                       Logger& logger);

    virtual void AsyncFlush() override;

    virtual void AsyncSend(InputEvent) override;

    virtual void AsyncClose() override;

   private:
    enum class FlushTrigger {
        Automatic = 0,
        Manual = 1,
    };
    using RequestType =
        boost::beast::http::request<boost::beast::http::string_body>;

    boost::asio::any_io_executor io_;
    Outbox outbox_;
    SummaryState summary_state_;

    std::chrono::milliseconds reaction_time_;
    std::chrono::milliseconds flush_interval_;
    boost::asio::steady_timer timer_;

    std::string host_;
    std::string path_;
    std::string authorization_;

    boost::uuids::random_generator uuids_;

    ConnPool conns_;

    bool full_outbox_encountered_;

    launchdarkly::ContextFilter filter_;

    Logger& logger_;

    void HandleSend(InputEvent e);

    std::optional<RequestType> MakeRequest();

    void Flush(FlushTrigger flush_type);

    void ScheduleFlush();

    std::vector<OutputEvent> Process(InputEvent e);
};

}  // namespace launchdarkly::events::detail
