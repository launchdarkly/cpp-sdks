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
#include "events/detail/conn_pool.hpp"
#include "events/detail/outbox.hpp"
#include "events/detail/summary_state.hpp"
#include "events/events.hpp"
#include "logger.hpp"
#include "context_filter.hpp"


namespace launchdarkly::events::detail {

class Dispatcher {
   public:
    Dispatcher(boost::asio::any_io_executor io,
               config::detail::Events const& config,
               config::ServiceHosts const& endpoints,
               std::string authorization,
               Logger& logger);

    void request_flush();

    void send(InputEvent);

    void shutdown();

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

    void handle_send(InputEvent e);

    std::optional<RequestType> make_request();

    void flush(FlushTrigger flush_type);

    void schedule_flush();

    std::vector<OutputEvent> process(InputEvent e);
};

}  // namespace launchdarkly::events::detail
