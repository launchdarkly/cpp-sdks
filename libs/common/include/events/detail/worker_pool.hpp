#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <chrono>
#include <functional>
#include <optional>
#include <vector>
#include "events/detail/request_worker.hpp"
#include "logger.hpp"
#include "network/detail/asio_requester.hpp"
#include "network/detail/http_requester.hpp"
namespace launchdarkly::events::detail {

/**
 * WorkerPool represents a pool of workers capable of delivering event payloads
 * to LaunchDarkly. In practice, this means initiating overlapping HTTP POSTs,
 * assuming more than one worker is active.
 *
 * A WorkerPool's executor is meant to be executed from a single thread.
 */
class WorkerPool {
   public:
    using PermanentFailureCallback = std::function<void()>;
    using ServerTimeCallback =
        std::function<void(std::chrono::system_clock::time_point)>;
    /**
     * Constructs a new WorkerPool.
     * @param io The executor used for all workers. Only safe for execution from
     * one thread.
     * @param pool_size How many workers are available.
     * @param delivery_retry_delay How long a worker should wait after a failed
     * delivery before trying again.
     * @param server_time_cb Invoked when a worker obtains a timestamp from the
     * server.
     * @param permanent_failure_callback Invoked once when any worker
     * experiences a permanent failure. Workers cannot be used after permanent
     * failure, so the pool should be regarded as unavailable after that point.
     * @param logger Logger for debug information.
     */
    WorkerPool(boost::asio::any_io_executor io,
               std::size_t pool_size,
               std::chrono::milliseconds delivery_retry_delay,
               ServerTimeCallback server_time_cb,
               PermanentFailureCallback permanent_failure_callback,
               Logger& logger);

    /**
     * Attempts to acquire a free worker from the pool (non-owning). If none
     * are available, returns nullptr. Only safe to call from the same thread
     * running the executor passed in the constructor.
     * @return
     */
    RequestWorker* AcquireWorker();

   private:
    std::vector<std::unique_ptr<RequestWorker>> workers_;
    bool permanent_failure_;
};

}  // namespace launchdarkly::events::detail
