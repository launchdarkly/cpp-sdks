#pragma once

#include <launchdarkly/async/cancellation.hpp>
#include <launchdarkly/async/promise.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>

#include <chrono>
#include <memory>

namespace launchdarkly::async {

// Returns a Future<bool> that resolves once the given duration elapses.
// The future resolves with true if the timer fired normally, or false if
// the timer was cancelled before it expired.
//
// If a CancellationToken is provided, cancelling the associated
// CancellationSource cancels the timer, resolving the future with false.
template <typename Rep, typename Period>
Future<bool> Delay(boost::asio::any_io_executor executor,
                   std::chrono::duration<Rep, Period> duration,
                   CancellationToken token = {}) {
    auto timer = std::make_shared<boost::asio::steady_timer>(executor);
    timer->expires_after(duration);
    Promise<bool> promise;
    auto future = promise.GetFuture();

    // cancel() is dispatched through post() rather than called directly to
    // satisfy ASIO's thread-safety requirement: steady_timer operations must
    // not be called concurrently from multiple threads.
    //
    // The callback is registered after async_wait is in flight so that an
    // already-cancelled token cancels the in-progress operation rather than
    // firing before async_wait has been called.
    //
    // The CancellationCallback is captured by the async_wait handler so its
    // lifetime matches the timer's; it is released (and deregistered) when
    // the handler fires.
    auto cb = std::make_shared<CancellationCallback>(
        std::move(token), [timer, executor] {
            boost::asio::post(executor, [timer] { timer->cancel(); });
        });

    timer->async_wait([p = std::move(promise), timer,
                       cb](boost::system::error_code code) mutable {
        cb.reset();
        p.Resolve(code != boost::asio::error::operation_aborted);
    });

    return future;
}

}  // namespace launchdarkly::async
