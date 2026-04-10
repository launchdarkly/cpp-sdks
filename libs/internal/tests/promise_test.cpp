#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <tl/expected.hpp>

#include <thread>

#include "launchdarkly/async/promise.hpp"

using namespace launchdarkly::async;

TEST(Promise, GetResultNotFinished) {
    Promise<int> promise;
    Future<int> future = promise.GetFuture();

    auto& result = future.GetResult();
    EXPECT_FALSE(result.has_value());
}

TEST(Promise, GetResultFinished) {
    Promise<int> promise;
    Future<int> future = promise.GetFuture();

    promise.Resolve(42);

    auto& result = future.GetResult();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
}

TEST(Promise, CopyOnlyResult) {
    struct CopyOnly {
        int value;
        explicit CopyOnly(int v) : value(v) {}
        CopyOnly(CopyOnly const&) = default;
        CopyOnly& operator=(CopyOnly const&) = default;
        CopyOnly(CopyOnly&&) = delete;
        CopyOnly& operator=(CopyOnly&&) = delete;
    };

    Promise<CopyOnly> promise;
    Future<CopyOnly> future = promise.GetFuture();

    promise.Resolve(CopyOnly{42});

    auto& result = future.GetResult();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->value, 42);
}

TEST(Promise, ContinueByReturningFuture) {
    Promise<int> promise1;
    Promise<int> promise2;
    Future<int> future2 = promise2.GetFuture();

    Future<int> chained = promise1.GetFuture().Then(
        [future2](int const&) { return future2; },
        [](Continuation<void()> f) { f(); });

    promise1.Resolve(0);
    promise2.Resolve(42);

    auto& result = chained.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
}

TEST(Promise, ResolvedBeforeContinuation) {
    Promise<int> promise;
    Future<int> future = promise.GetFuture();

    promise.Resolve(21);

    Future<int> future2 = future.Then(
        [](int const& val) { return val * 2; },
        [](Continuation<void()> f) { f(); });

    auto& result = future2.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
}

TEST(Promise, ResolvedAfterContinuation) {
    Promise<int> promise;
    Future<int> future = promise.GetFuture();

    Future<int> future2 = future.Then(
        [](int const& val) { return val * 2; },
        [](Continuation<void()> f) { f(); });

    promise.Resolve(21);

    auto& result = future2.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
}

TEST(Promise, MoveOnlyResult) {
    Promise<std::unique_ptr<int>> promise;
    Future<std::unique_ptr<int>> future = promise.GetFuture();

    promise.Resolve(std::make_unique<int>(42));

    auto& result = future.GetResult();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(**result, 42);
}

// Verifies that a continuation which captures a copy-only type (deleted move
// constructor) can still be registered and invoked correctly.
TEST(Promise, CopyOnlyCallback) {
    struct CopyOnlyInt {
        int value;
        explicit CopyOnlyInt(int v) : value(v) {}
        CopyOnlyInt(CopyOnlyInt const&) = default;
        CopyOnlyInt& operator=(CopyOnlyInt const&) = default;
        CopyOnlyInt(CopyOnlyInt&&) = delete;
        CopyOnlyInt& operator=(CopyOnlyInt&&) = delete;
    };

    Promise<int> promise;
    Future<int> future = promise.GetFuture();

    CopyOnlyInt multiplier{2};
    Future<int> future2 = future.Then(
        [multiplier](int const& val) { return val * multiplier.value; },
        [](Continuation<void()> f) { f(); });

    promise.Resolve(21);

    auto& result = future2.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
}

// Verifies that a continuation which captures a move-only type (deleted copy
// constructor) can still be registered and invoked correctly.
TEST(Promise, MoveOnlyCallback) {
    Promise<int> promise;
    Future<int> future = promise.GetFuture();

    auto captured = std::make_unique<int>(2);
    Future<int> future2 = future.Then(
        [captured = std::move(captured)](int const& val) {
            return val * *captured;
        },
        [](Continuation<void()> f) { f(); });

    promise.Resolve(21);

    auto& result = future2.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
}

// Verifies that when T is both moveable and copyable, resolving a promise
// moves the value into storage rather than copying it.
TEST(Promise, ResultMovedWhenPossible) {
    int copies = 0;
    struct Counted {
        int value;
        int* copy_count;
        explicit Counted(int v, int* c) : value(v), copy_count(c) {}
        Counted(Counted const& o) : value(o.value), copy_count(o.copy_count) { ++(*copy_count); }
        Counted& operator=(Counted const& o) { value = o.value; copy_count = o.copy_count; ++(*copy_count); return *this; }
        Counted(Counted&&) noexcept = default;
        Counted& operator=(Counted&&) noexcept = default;
    };

    Promise<Counted> promise;
    Future<Counted> future = promise.GetFuture();

    promise.Resolve(Counted{42, &copies});

    auto& result = future.GetResult();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->value, 42);
    EXPECT_EQ(copies, 0);
}

// Verifies that when a continuation is both moveable and copyable, Then stores
// it by moving rather than copying.
TEST(Promise, CallbackMovedWhenPossible) {
    int copies = 0;
    struct Counted {
        int value;
        int* copy_count;
        explicit Counted(int v, int* c) : value(v), copy_count(c) {}
        Counted(Counted const& o) : value(o.value), copy_count(o.copy_count) { ++(*copy_count); }
        Counted& operator=(Counted const& o) { value = o.value; copy_count = o.copy_count; ++(*copy_count); return *this; }
        Counted(Counted&&) noexcept = default;
        Counted& operator=(Counted&&) noexcept = default;
    };

    Promise<int> promise;
    Future<int> future = promise.GetFuture();

    Counted multiplier{2, &copies};
    copies = 0;  // reset after construction

    future.Then(
        [multiplier = std::move(multiplier)](int const& val) mutable {
            return val * multiplier.value;
        },
        [](Continuation<void()> f) { f(); });

    EXPECT_EQ(copies, 0);

    promise.Resolve(21);
}

// Demonstrates using std::monostate as a void-like result type for fire-and-forget
// async operations where the completion matters but no value is produced.
TEST(Promise, MonostateVoidLike) {
    Promise<std::monostate> promise;
    Future<std::monostate> future = promise.GetFuture();

    bool ran = false;
    future.Then(
        [&ran](std::monostate const&) {
            ran = true;
            return std::monostate{};
        },
        [](Continuation<void()> f) { f(); });

    promise.Resolve(std::monostate{});

    auto& result = future.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(ran);
}

// Demonstrates using tl::expected as T to represent a fallible async operation
// that resolves successfully.
TEST(Promise, ExpectedSuccess) {
    Promise<tl::expected<int, std::string>> promise;
    Future<tl::expected<int, std::string>> future = promise.GetFuture();

    promise.Resolve(42);

    auto& result = future.GetResult();
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->has_value());
    EXPECT_EQ(**result, 42);
}

// Demonstrates using tl::expected as T to represent a fallible async operation
// that resolves with an error.
TEST(Promise, ExpectedFailure) {
    Promise<tl::expected<int, std::string>> promise;
    Future<tl::expected<int, std::string>> future = promise.GetFuture();

    promise.Resolve(tl::unexpected(std::string("timed out")));

    auto& result = future.GetResult();
    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->has_value());
    EXPECT_EQ(result->error(), "timed out");
}


TEST(Promise, SimplePromise) {
    Promise<int> promise;
    Future<int> future = promise.GetFuture();

    Future<float> future2 = future.Then(
        [](int const& inner) { return static_cast<float>(inner * 2.0); },
        [](Continuation<void()> f) { f(); });

    promise.Resolve(43);

    auto& result = future2.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(result.has_value());
    EXPECT_FLOAT_EQ(*result, 86.0f);
}

TEST(Promise, ASIOTest) {
    boost::asio::io_context ioc;
    auto work = boost::asio::make_work_guard(ioc);
    std::thread ioc_thread([&]() { ioc.run(); });

    Promise<int> promise;
    Future<int> future = promise.GetFuture();

    Future<float> future2 = future.Then(
        [](int const& inner) { return static_cast<float>(inner * 2.0); },
        [&ioc](Continuation<void()> f) {
            boost::asio::post(ioc, [f = std::move(f)]() mutable { f(); });
        });

    promise.Resolve(42);

    auto& result = future2.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(result.has_value());
    EXPECT_FLOAT_EQ(*result, 84.0f);

    work.reset();
    ioc_thread.join();
}
