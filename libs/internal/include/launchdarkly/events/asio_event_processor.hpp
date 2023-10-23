#pragma once

#include <launchdarkly/config/shared/built/events.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/context_filter.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/network/http_requester.hpp>

#include <launchdarkly/events/data/events.hpp>
#include <launchdarkly/events/detail/event_batch.hpp>
#include <launchdarkly/events/detail/lru_cache.hpp>
#include <launchdarkly/events/detail/outbox.hpp>
#include <launchdarkly/events/detail/summarizer.hpp>
#include <launchdarkly/events/detail/worker_pool.hpp>
#include <launchdarkly/events/event_processor_interface.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/http.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <chrono>
#include <optional>
#include <tuple>

namespace launchdarkly::events {

template <typename SDK>
class AsioEventProcessor : public IEventProcessor {
   public:
    AsioEventProcessor(
        boost::asio::any_io_executor const& io,
        config::shared::built::ServiceEndpoints const& endpoints,
        config::shared::built::Events const& events_config,
        config::shared::built::HttpProperties const& http_properties,
        Logger& logger);

    virtual void FlushAsync() override;

    virtual void SendAsync(events::InputEvent event) override;

    virtual void ShutdownAsync() override;

   private:
    using Clock = std::chrono::system_clock;
    enum class FlushTrigger {
        Automatic = 0,
        Manual = 1,
    };

    boost::asio::any_io_executor io_;
    detail::Outbox outbox_;
    detail::Summarizer summarizer_;

    std::chrono::milliseconds flush_interval_;
    boost::asio::steady_timer timer_;

    std::string url_;

    config::shared::built::HttpProperties http_props_;

    boost::uuids::random_generator uuids_;

    detail::WorkerPool workers_;

    std::size_t inbox_capacity_;
    std::size_t inbox_size_;
    std::mutex inbox_mutex_;

    bool full_outbox_encountered_;
    bool full_inbox_encountered_;
    bool permanent_delivery_failure_;

    std::optional<Clock::time_point> last_known_past_time_;

    launchdarkly::ContextFilter filter_;

    detail::LRUCache context_key_cache_;

    Logger& logger_;

    void HandleSend(InputEvent event);

    std::optional<detail::EventBatch> CreateBatch();

    void Flush(FlushTrigger flush_type);

    void ScheduleFlush();

    std::vector<OutputEvent> Process(InputEvent event);

    bool InboxIncrement();
    void InboxDecrement();

    void OnEventDeliveryResult(std::size_t count,
                               detail::RequestWorker::DeliveryResult);
};

}  // namespace launchdarkly::events
