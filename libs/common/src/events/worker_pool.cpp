#include "events/detail/worker_pool.hpp"
#include <memory>
#include "events/detail/request_worker.hpp"

namespace launchdarkly::events::detail {

WorkerPool::WorkerPool(boost::asio::any_io_executor io,
                       std::size_t pool_size,
                       std::chrono::milliseconds delivery_retry_delay,
                       ServerTimeCallback server_time_cb,
                       Logger& logger)
    : io_(io), workers_() {
    // Provide a default no-op behavior if no server time callback is specified
    // (since this will be invoked quite often.)
    auto server_time_callback =
        server_time_cb ? server_time_cb : [](auto server_time) {};

    for (std::size_t i = 0; i < pool_size; i++) {
        workers_.emplace_back(std::make_unique<RequestWorker>(
            io_, delivery_retry_delay, server_time_callback, logger));
    }
}

}  // namespace launchdarkly::events::detail
