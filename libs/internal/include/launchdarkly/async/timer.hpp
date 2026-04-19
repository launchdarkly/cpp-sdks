#pragma once

#include <launchdarkly/async/promise.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>

#include <chrono>
#include <memory>

namespace launchdarkly::async {

// Returns a Future<bool> that resolves once the given duration elapses.
// The future resolves with true if the timer fired normally, or false if
// the timer was cancelled before it expired.
template <typename Rep, typename Period>
Future<bool> Delay(boost::asio::any_io_executor executor,
                   std::chrono::duration<Rep, Period> duration) {
    auto timer = std::make_shared<boost::asio::steady_timer>(executor);
    timer->expires_after(duration);
    Promise<bool> promise;
    auto future = promise.GetFuture();
    timer->async_wait([p = std::move(promise),
                       timer](boost::system::error_code code) mutable {
        p.Resolve(code != boost::asio::error::operation_aborted);
    });
    return future;
}

}  // namespace launchdarkly::async
