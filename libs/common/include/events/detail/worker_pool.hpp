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
    using ServerTimeCallback =
        std::function<void(std::chrono::system_clock::time_point)>;
    /**
     * Constructs a new WorkerPool.
     * @param io The executor used for all workers.
     * @param pool_size How many workers to make available.
     * @param delivery_retry_delay How long a worker should wait after a failed
     * delivery before trying again.
     * @param logger Logger.
     */
    WorkerPool(boost::asio::any_io_executor io,
               std::size_t pool_size,
               std::chrono::milliseconds delivery_retry_delay,
               Logger& logger);

    /**
     * Attempts to find a free worker. If none are available, the completion
     * handler is invoked with nullptr.
     */
    template <typename CompletionToken>
    auto Get(CompletionToken&& token) {
        namespace asio = boost::asio;
        namespace system = boost::system;

        using Sig = void(RequestWorker*);
        using Result = asio::async_result<std::decay_t<CompletionToken>, Sig>;
        using Handler = typename Result::completion_handler_type;

        Handler handler(std::forward<decltype(token)>(token));
        Result result(handler);

        boost::asio::dispatch(io_, [this, handler]() mutable {
            for (auto& worker : workers_) {
                if (worker->Available()) {
                    handler(worker.get());
                    return;
                }
            }
            handler(nullptr);
        });

        return result.get();
    }

   private:
    boost::asio::any_io_executor io_;
    std::vector<std::unique_ptr<RequestWorker>> workers_;
};

}  // namespace launchdarkly::events::detail
