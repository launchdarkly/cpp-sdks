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

    // This code is tricky because there are a few constraints that conflict.
    // 1. We need to make sure timer->cancel isn't called _before_
    //    timer->async_wait, or else it'll just be ignored.
    // 2. The cancellation_callback has to be created _before_
    //    timer->async_wait, because it has to be captured by async_wait's
    //    handler, because it is an RAII type, and once it is destroyed, it
    //    deregisters itself. It has to stay alive as long as the timer needs to
    //    be cancellable.

    Promise<std::monostate> timer_started_promise;
    Future<std::monostate> timer_started_future =
        timer_started_promise.GetFuture();

    auto cancel_timer = [timer, executor,
                         timer_started_future =
                             std::move(timer_started_future)]() mutable {
        timer_started_future.Then(
            [timer](auto const&) -> std::monostate {
                timer->cancel();
                return {};
            },
            [executor](Continuation<void()> f) {
                boost::asio::post(executor, f);
            });
    };

    auto cancellation_callback =
        std::make_shared<CancellationCallback>(std::move(token), cancel_timer);

    timer->async_wait([p = std::move(promise), timer, cancellation_callback](
                          boost::system::error_code code) mutable {
        cancellation_callback.reset();
        p.Resolve(code != boost::asio::error::operation_aborted);
    });

    timer_started_promise.Resolve({});

    return future;
}

}  // namespace launchdarkly::async
