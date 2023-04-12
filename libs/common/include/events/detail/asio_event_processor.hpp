#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/http.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <chrono>
#include <optional>
#include "config/detail/events.hpp"
#include "config/detail/service_endpoints.hpp"
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
    AsioEventProcessor(boost::asio::any_io_executor const& io,
                       config::detail::Events const& config,
                       config::ServiceEndpoints const& endpoints,
                       std::string authorization,
                       Logger& logger);

    void AsyncFlush() override;

    void AsyncSend(InputEvent event) override;

    void AsyncClose() override;

   private:
    enum class FlushTrigger {
        Automatic = 0,
        Manual = 1,
    };
    using RequestType =
        boost::beast::http::request<boost::beast::http::string_body>;

    boost::asio::any_io_executor io_;
    Outbox outbox_;
    Summarizer summary_state_;

    std::chrono::milliseconds flush_interval_;
    boost::asio::steady_timer timer_;

    std::string host_;
    std::string path_;
    std::string authorization_;

    boost::uuids::random_generator uuids_;

    ConnPool conns_;

    std::size_t inbox_capacity_;
    std::size_t inbox_size_;
    std::mutex inbox_mutex_;

    bool full_outbox_encountered_;
    bool full_inbox_encountered_;

    launchdarkly::ContextFilter filter_;

    Logger& logger_;

    void HandleSend(InputEvent event);

    std::optional<RequestType> MakeRequest();

    void Flush(FlushTrigger flush_type);

    void ScheduleFlush();

    std::vector<OutputEvent> Process(InputEvent event);

    bool InboxIncrement();
    void InboxDecrement();
};

}  // namespace launchdarkly::events::detail
