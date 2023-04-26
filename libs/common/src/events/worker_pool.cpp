#include "events/detail/worker_pool.hpp"
#include <memory>
#include "events/detail/request_worker.hpp"

namespace launchdarkly::events::detail {

WorkerPool::WorkerPool(boost::asio::any_io_executor io,
                       std::size_t pool_size,
                       std::chrono::milliseconds delivery_retry_delay,
                       ServerTimeCallback server_time_cb,
                       PermanentFailureCallback permanent_failure_callback,
                       Logger& logger)
    : workers_(), permanent_failure_(false) {
    auto permanent_failure_once = [=]() {
        if (!permanent_failure_) {
            permanent_failure_callback();
            permanent_failure_ = true;
        }
    };
    for (std::size_t i = 0; i < pool_size; i++) {
        workers_.emplace_back(std::make_unique<RequestWorker>(
            io, delivery_retry_delay, server_time_cb, permanent_failure_once,
            logger));
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
