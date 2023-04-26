#include "events/detail/worker_pool.hpp"
#include <memory>
#include "events/detail/request_worker.hpp"

namespace launchdarkly::events::detail {

WorkerPool::WorkerPool(boost::asio::any_io_executor io,
                       std::size_t pool_size,
                       std::chrono::milliseconds delivery_retry_delay,
                       ServerTimeCallback server_time_cb,
                       PermanentFailureCallback permanent_failure_cb,
                       Logger& logger)
    : logger_(logger), io_(io), workers_(), permanent_failure_(false) {
    // Provide a default logging behavior if no permanent failure callback is
    // specified.
    auto permanent_failure_callback =
        permanent_failure_cb ? permanent_failure_cb : [this]() {
            LD_LOG(logger_, LogLevel::kInfo)
                << "Error posting <number of events in payload> event(s) "
                   "(giving "
                   "up permanently): HTTP error <status>";
        };

    // Provide a default no-op behavior if no server time callback is specified
    // (since this will be invoked quite often.)
    auto server_time_callback =
        server_time_cb ? server_time_cb : [](auto server_time) {};

    // Wrap the permanent failure callback, so it is only executed once from
    // perspective of the pool's user, even if multiple workers call it.
    auto permanent_failure_once = [=]() {
        if (!permanent_failure_) {
            permanent_failure_callback();
            permanent_failure_ = true;
        }
    };

    for (std::size_t i = 0; i < pool_size; i++) {
        workers_.emplace_back(std::make_unique<RequestWorker>(
            io_, delivery_retry_delay, server_time_callback,
            permanent_failure_once, logger));
    }
}

}  // namespace launchdarkly::events::detail
