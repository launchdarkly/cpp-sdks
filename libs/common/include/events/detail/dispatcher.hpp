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
#include "events/detail/conn_pool.hpp"
#include "events/detail/outbox.hpp"
#include "events/detail/summary_state.hpp"
#include "events/events.hpp"
#include "logger.hpp"

namespace launchdarkly::events::detail {

class Dispatcher {
   public:
    Dispatcher(boost::asio::any_io_executor io,
               config::detail::Events const& config,
               config::ServiceEndpoints const& endpoints,
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
