#include "events/detail/worker_pool.hpp"
#include <memory>
#include "events/detail/request_worker.hpp"

namespace launchdarkly::events::detail {

WorkerPool::WorkerPool(boost::asio::any_io_executor io,
                       std::size_t pool_size,
                       std::chrono::milliseconds delivery_retry_delay,
                       Logger& logger)
    : io_(io), workers_() {
    for (std::size_t i = 0; i < pool_size; i++) {
        workers_.emplace_back(
            std::make_unique<RequestWorker>(io_, delivery_retry_delay, logger));
    }
}

}  // namespace launchdarkly::events::detail
