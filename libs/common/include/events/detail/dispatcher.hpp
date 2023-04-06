#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/http.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <chrono>
#include <optional>
#include "events/detail/conn_pool.hpp"
#include "events/detail/outbox.hpp"
#include "events/detail/summary_state.hpp"
#include "events/events.hpp"
#include "logger.hpp"

namespace launchdarkly::events::detail {

class Dispatcher {
   public:
    /**
     *
     * Capacity and interval are related. Capacity is the amount of events that
     * can accumulate between flushes. If the application is producing events
     * faster than flushing, then the outbox will become full and be unable
     * to accept new items. At that point, events will be dropped. To guard
     * against this, lower the flush interval to the point where events are no
     * longer dropped.
     * @param io
     * @param capacity
     * @param reaction_time
     * @param flush_interval
     * @param endpoint_host
     * @param endpoint_path
     * @param authorization
     * @param logger
     */
    Dispatcher(boost::asio::any_io_executor io,
               std::size_t outbox_capacity,
               std::chrono::milliseconds flush_interval,
               std::string endpoint_host,
               std::string endpoint_path,
               std::string authorization,
               Logger& logger);

    void request_flush();

    void send(InputEvent);
    void shutdown();

   private:
    using RequestType =
        boost::beast::http::request<boost::beast::http::string_body>;

    boost::asio::any_io_executor io_;
    boost::asio::executor_work_guard<boost::asio::any_io_executor> work_guard_;
    Outbox outbox_;
    SummaryState summary_state_;

    std::chrono::milliseconds reaction_time_;
    std::chrono::milliseconds min_flush_interval_;
    std::chrono::system_clock::time_point last_flush_;

    std::string host_;
    std::string path_;
    std::string authorization_;

    boost::uuids::random_generator uuids_;

    ConnPool conns_;

    bool full_outbox_encountered_;

    Logger& logger_;

    void handle_send(InputEvent e);

    std::optional<RequestType> make_request();

    void flush(std::chrono::system_clock::time_point when);
    bool flush_due();

    std::vector<OutputEvent> process(InputEvent e);
};

}  // namespace launchdarkly::events::detail
