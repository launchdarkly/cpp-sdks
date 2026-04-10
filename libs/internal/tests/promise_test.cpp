#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

#include <thread>

#include "launchdarkly/async/promise.hpp"

using namespace launchdarkly::async;

// Cases to cover:
// Resolved after continuation.
// Resolved before continuation.
// Continue with value.
// Continue with promise.
// A result that can be copied but not moved.
// A result that can be moved but not copied.
// A callback that can be copied but not moved.
// A callback that can be moved but not copied.
// Test that results are moved if possible, even if they could be copied.
// Test that callbacks are moved if possible, even if they could be copied.

TEST(Promise, SimplePromise) {
    boost::asio::io_context ioc;
    auto work = boost::asio::make_work_guard(ioc);
    std::thread ioc_thread([&]() { ioc.run(); });

    int result = 0;

    Promise<int> promise;
    Future<int> future = promise.GetFuture();

    Future<float> future2 = future.Then(
        [&](int const& inner) {
            result = 42;
            return static_cast<float>(result);
        },
        [](Continuation<void()> f) { f(); });

    promise.Resolve(43);

    bool work_ran = false;
    boost::asio::post(ioc, [&]() { work_ran = true; });

    work.reset();
    ioc_thread.join();

    ASSERT_TRUE(work_ran);
}
