#include "events/detail/worker_pool.hpp"
#include <memory>
#include "events/detail/request_worker.hpp"

namespace launchdarkly::events::detail {

WorkerPool::WorkerPool(boost::asio::any_io_executor io,
                       Logger& logger,
                       std::size_t pool_size,
                       std::chrono::milliseconds delivery_retry_delay,
                       ServerTimeCallback server_time_cb,
                       PermanentFailureCallback permanent_failure_callback) {
    for (std::size_t i = 0; i < pool_size; i++) {
        workers_.emplace_back(std::make_unique<RequestWorker>(
            io, delivery_retry_delay, server_time_cb,
            permanent_failure_callback, logger));
    }
}

RequestWorker* WorkerPool::AcquireWorker() {
    for (auto& worker : workers_) {
        if (worker->Available()) {
            return worker.get();
        }
    }
    return nullptr;
}
}  // namespace launchdarkly::events::detail
