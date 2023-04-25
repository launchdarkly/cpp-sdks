#pragma once

#include <boost/asio/any_io_executor.hpp>
#include "logger.hpp"
#include "network/detail/asio_requester.hpp"
#include "network/detail/http_requester.hpp"

#include <chrono>
namespace launchdarkly::events::detail {

class ConnPool {
   public:
    using RequestType = network::detail::HttpRequest;
    ConnPool(boost::asio::any_io_executor io, Logger& logger);
    void Deliver(RequestType request);
    using Clock = std::chrono::system_clock;

    bool PermanentFailure() const;

    std::optional<Clock::time_point> LastKnownPastTime() const;

   private:
    Logger& logger_;
    network::detail::AsioRequester requester_;

    bool permanent_failure_;

    // TODO(cwaldren): protect w/mutex or pass in a callback or something
    std::optional<Clock::time_point> last_known_past_time_;
};
}  // namespace launchdarkly::events::detail
