#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/http.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <chrono>
#include <optional>
#include <tuple>

#include <launchdarkly/config/shared/built/events.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/context_filter.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/network/http_requester.hpp>

#include "event_batch.hpp"
#include "events.hpp"
#include "outbox.hpp"
#include "summarizer.hpp"
#include "worker_pool.hpp"

namespace launchdarkly::events {

template <typename SDK>
class AsioEventProcessor {
   public:
    AsioEventProcessor(
        boost::asio::any_io_executor const& io,
        std::string const& sdk_key,
        config::shared::built::ServiceEndpoints const& endpoints,
        config::shared::built::Events const& events_config,
        config::shared::built::HttpProperties const& http_properties,
        Logger& logger);

    void AsyncFlush();

    void AsyncSend(InputEvent event);

    void AsyncClose();

   private:
    using Clock = std::chrono::system_clock;
    enum class FlushTrigger {
        Automatic = 0,
        Manual = 1,
    };

    boost::asio::any_io_executor io_;
    Outbox outbox_;
    Summarizer summarizer_;

    std::chrono::milliseconds flush_interval_;
    boost::asio::steady_timer timer_;

    std::string url_;
    std::string authorization_;

    config::shared::built::HttpProperties http_props_;

    boost::uuids::random_generator uuids_;

    WorkerPool workers_;

    std::size_t inbox_capacity_;
    std::size_t inbox_size_;
    std::mutex inbox_mutex_;

    bool full_outbox_encountered_;
    bool full_inbox_encountered_;
    bool permanent_delivery_failure_;

    std::optional<Clock::time_point> last_known_past_time_;

    launchdarkly::ContextFilter filter_;

    Logger& logger_;

    void HandleSend(InputEvent event);

    std::optional<EventBatch> CreateBatch();

    void Flush(FlushTrigger flush_type);

    void ScheduleFlush();

    std::vector<OutputEvent> Process(InputEvent event);

    bool InboxIncrement();
    void InboxDecrement();

    void OnEventDeliveryResult(std::size_t count,
                               RequestWorker::DeliveryResult);
};

}  // namespace launchdarkly::events
