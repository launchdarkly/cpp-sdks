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

class ConnPool {
   public:
    using RequestType = network::detail::HttpRequest;
    using PermanentFailureCallback = std::function<void()>;
    using ServerTimeCallback =
        std::function<void(std::chrono::system_clock::time_point)>;
    ConnPool(boost::asio::any_io_executor io,
             Logger& logger,
             std::size_t pool_size,
             std::chrono::milliseconds delivery_retry_delay,
             ServerTimeCallback server_time_cb,
             PermanentFailureCallback permanent_failure_callback);
    void Deliver(RequestType request);
    using Clock = std::chrono::system_clock;

    bool PermanentFailure() const;

    RequestWorker* AcquireWorker();

    std::optional<Clock::time_point> LastKnownPastTime() const;

   private:
    Logger& logger_;

    std::vector<RequestWorker> workers_;
};

}  // namespace launchdarkly::events::detail
