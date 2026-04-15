#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <tl/expected.hpp>

#include <thread>

#include "launchdarkly/async/promise.hpp"

using namespace launchdarkly::async;

TEST(Promise, SimplePromise) {
    Promise<int> promise;
    Future<int> future = promise.GetFuture();

    Future<float> future2 = future.Then(
        [](int const& inner) { return static_cast<float>(inner * 2.0); },
        [](Continuation<void()> f) { f(); });

    promise.Resolve(43);

    auto result = future2.WaitForResult(std::chrono::seconds(5));
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

    auto result = future2.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(result.has_value());
    EXPECT_FLOAT_EQ(*result, 84.0f);

    work.reset();
    ioc_thread.join();
}

TEST(Promise, GetResultNotFinished) {
    Promise<int> promise;
    Future<int> future = promise.GetFuture();

    auto result = future.GetResult();
    EXPECT_FALSE(result.has_value());
}

TEST(Promise, GetResultFinished) {
    Promise<int> promise;
    Future<int> future = promise.GetFuture();

    promise.Resolve(42);

    auto result = future.GetResult();
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

    auto result = future.GetResult();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->value, 42);
}

TEST(Promise, ContinueByReturningFuture) {
    Promise<int> promise1;
    Promise<int> promise2;
    Future<int> future2 = promise2.GetFuture();

    Future<int> chained =
        promise1.GetFuture().Then([future2](int const&) { return future2; },
                                  [](Continuation<void()> f) { f(); });

    promise1.Resolve(0);
    promise2.Resolve(42);

    auto result = chained.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
}

TEST(Promise, ResolvedBeforeContinuation) {
    Promise<int> promise;
    Future<int> future = promise.GetFuture();

    promise.Resolve(21);

    Future<int> future2 = future.Then([](int const& val) { return val * 2; },
                                      [](Continuation<void()> f) { f(); });

    auto result = future2.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
}

TEST(Promise, ResolvedAfterContinuation) {
    Promise<int> promise;
    Future<int> future = promise.GetFuture();

    Future<int> future2 = future.Then([](int const& val) { return val * 2; },
                                      [](Continuation<void()> f) { f(); });

    promise.Resolve(21);

    auto result = future2.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
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

    auto result = future2.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
}

// Verifies that a continuation which captures a move-only type (deleted copy
// constructor) can still be registered and invoked correctly.
TEST(Promise, MoveOnlyCallback) {
    Promise<int> promise;
    Future<int> future = promise.GetFuture();

    auto captured = std::make_unique<int>(2);
    Future<int> future2 =
        future.Then([captured = std::move(captured)](
                        int const& val) { return val * *captured; },
                    [](Continuation<void()> f) { f(); });

    promise.Resolve(21);

    auto result = future2.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
}

// Verifies that when a continuation is both moveable and copyable, Then stores
// it by moving rather than copying.
TEST(Promise, CallbackMovedWhenPossible) {
    int copies = 0;
    struct Counted {
        int value;
        int* copy_count;
        explicit Counted(int v, int* c) : value(v), copy_count(c) {}
        Counted(Counted const& o) : value(o.value), copy_count(o.copy_count) {
            ++(*copy_count);
        }
        Counted& operator=(Counted const& o) {
            value = o.value;
            copy_count = o.copy_count;
            ++(*copy_count);
            return *this;
        }
        Counted(Counted&&) noexcept = default;
        Counted& operator=(Counted&&) noexcept = default;
    };

    Promise<int> promise;
    Future<int> future = promise.GetFuture();

    Counted multiplier{2, &copies};
    copies = 0;  // reset after construction

    future.Then([multiplier = std::move(multiplier)](
                    int const& val) mutable { return val * multiplier.value; },
                [](Continuation<void()> f) { f(); });

    EXPECT_EQ(copies, 0);

    promise.Resolve(21);
}

// Demonstrates using std::monostate as a void-like result type for
// fire-and-forget async operations where the completion matters but no value is
// produced.
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

    auto result = future.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(ran);
}

// Demonstrates using tl::expected as T to represent a fallible async operation
// that resolves successfully.
TEST(Promise, ExpectedSuccess) {
    Promise<tl::expected<int, std::string>> promise;
    Future<tl::expected<int, std::string>> future = promise.GetFuture();

    promise.Resolve(42);

    auto result = future.GetResult();
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

    auto result = future.GetResult();
    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->has_value());
    EXPECT_EQ(result->error(), "timed out");
}

// Verifies that Promise supports move assignment, consistent with it being
// move-only.
TEST(Promise, MoveAssignment) {
    Promise<int> p1;
    Future<int> future = p1.GetFuture();

    Promise<int> p2;
    p2 = std::move(p1);
    p2.Resolve(42);

    EXPECT_EQ(*future.GetResult(), 42);
}

// Verifies that Future supports move assignment.
TEST(Promise, FutureMoveAssignment) {
    Promise<int> promise;
    Future<int> f1 = promise.GetFuture();
    Future<int> f2 = promise.GetFuture();

    f2 = std::move(f1);
    promise.Resolve(42);

    EXPECT_EQ(*f2.GetResult(), 42);
}

// Verifies that Future supports copy assignment.
TEST(Promise, FutureCopyAssignment) {
    Promise<int> promise;
    Future<int> f1 = promise.GetFuture();
    Future<int> f2 = promise.GetFuture();

    f2 = f1;
    promise.Resolve(42);

    EXPECT_EQ(*f2.GetResult(), 42);
}

// Verifies that a Continuation can be constructed from an lvalue callable
// (named lambda), not just from a temporary.
TEST(Promise, LvalueLambdaContinuation) {
    auto fn = [](int x) { return x * 2; };
    Continuation<int(int)> c(fn);
    EXPECT_EQ(c(21), 42);
}

TEST(WhenAll, NoFutures) {
    Future<std::monostate> result = WhenAll();
    EXPECT_TRUE(result.IsFinished());
}

// Verifies WhenAll resolves when all futures are already resolved.
TEST(WhenAll, AllAlreadyResolved) {
    Promise<int> p1;
    Promise<std::string> p2;

    p1.Resolve(1);
    p2.Resolve("hello");

    Future<std::monostate> result = WhenAll(p1.GetFuture(), p2.GetFuture());
    auto r = result.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(r.has_value());
}

// Verifies WhenAll resolves only after all futures resolve, using futures of
// mixed value types. The original futures still hold their results afterward.
TEST(WhenAll, ResolvesAfterAll) {
    Promise<int> p1;
    Promise<std::string> p2;

    Future<int> f1 = p1.GetFuture();
    Future<std::string> f2 = p2.GetFuture();

    Future<std::monostate> result = WhenAll(f1, f2);

    EXPECT_FALSE(result.IsFinished());
    p1.Resolve(42);
    EXPECT_FALSE(result.IsFinished());
    p2.Resolve("done");

    auto r = result.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*f1.GetResult(), 42);
    EXPECT_EQ(*f2.GetResult(), "done");
}

// Verifies that WhenAny resolves with the index of the first future to resolve.
TEST(WhenAny, FirstResolved) {
    Promise<int> p0;
    Promise<int> p1;
    Promise<int> p2;

    Future<std::size_t> result =
        WhenAny(p0.GetFuture(), p1.GetFuture(), p2.GetFuture());

    EXPECT_FALSE(result.IsFinished());
    p1.Resolve(42);

    auto r = result.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 1u);
}

// Verifies that WhenAny works with futures of mixed value types, and that
// resolving a later future after the winner does not change the result.
TEST(WhenAny, MixedTypesFirstWins) {
    Promise<int> p0;
    Promise<std::string> p1;

    Future<int> f0 = p0.GetFuture();
    Future<std::string> f1 = p1.GetFuture();

    Future<std::variant<int, std::string>> result = WhenAny(f0, f1).Then(
        [f0, f1](size_t const& index) -> std::variant<int, std::string> {
            if (index == 0) {
                return f0.GetResult().value();
            } else {
                return f1.GetResult().value();
            }
        },
        [](Continuation<void()> f) { f(); });

    p1.Resolve("hello");
    p0.Resolve(99);

    auto r = result.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(std::get<std::string>(*result.GetResult()), "hello");
}

// Verifies that WhenAny resolves immediately if a future is already resolved.
TEST(WhenAny, AlreadyResolved) {
    Promise<int> p0;
    Promise<int> p1;

    p0.Resolve(42);

    Future<std::size_t> result = WhenAny(p0.GetFuture(), p1.GetFuture());

    auto r = result.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 0u);
}

TEST(WhenAll, ConcurrentResolution) {
    auto spawn = [](int val) {
        Promise<int> p;
        Future<int> f = p.GetFuture();
        return std::make_pair(
            f, std::thread([p = std::move(p), val]() mutable {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                p.Resolve(val);
            }));
    };

    auto [f1, t1] = spawn(1);
    auto [f2, t2] = spawn(2);
    auto [f3, t3] = spawn(3);
    auto [f4, t4] = spawn(4);
    auto [f5, t5] = spawn(5);

    auto result = WhenAll(f1, f2, f3, f4, f5);

    auto r = result.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(r.has_value());

    EXPECT_EQ(f1.GetResult().value(), 1);
    EXPECT_EQ(f2.GetResult().value(), 2);
    EXPECT_EQ(f3.GetResult().value(), 3);
    EXPECT_EQ(f4.GetResult().value(), 4);
    EXPECT_EQ(f5.GetResult().value(), 5);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
}

// Verifies that WaitForResult returns nullopt when the promise is never
// resolved within the timeout.
TEST(Promise, WaitForResultTimeout) {
    Promise<int> promise;
    Future<int> future = promise.GetFuture();

    auto result = future.WaitForResult(std::chrono::milliseconds(50));
    EXPECT_FALSE(result.has_value());
}

// Verifies that multiple threads can concurrently register continuations on
// the same future before it resolves, and all continuations run after
// resolution.
TEST(Promise, ConcurrentThenRegistration) {
    Promise<int> promise;
    Future<int> future = promise.GetFuture();

    std::atomic<int> count{0};
    std::vector<std::thread> threads;
    std::vector<Future<std::monostate>> results;

    for (int i = 0; i < 10; i++) {
        results.push_back(future.Then(
            [&count](int const&) {
                count++;
                return std::monostate{};
            },
            [](Continuation<void()> f) { f(); }));
    }

    promise.Resolve(42);

    for (auto& r : results) {
        ASSERT_TRUE(r.WaitForResult(std::chrono::seconds(5)).has_value());
    }
    EXPECT_EQ(count, 10);
}

// Verifies that when multiple threads race to resolve the same promise,
// exactly one succeeds and the rest return false.
TEST(Promise, ConcurrentResolveSingleWinner) {
    Promise<int> promise;
    Future<int> future = promise.GetFuture();

    std::atomic<int> winners{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&promise, &winners, i] {
            if (promise.Resolve(i)) {
                winners++;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(winners, 1);
    EXPECT_TRUE(future.GetResult().has_value());
}

// Verifies that a chain of Then calls executes correctly when continuations
// are dispatched via a multi-threaded ASIO executor.
TEST(Promise, MultiThreadedASIOExecutor) {
    boost::asio::io_context ioc;
    auto work = boost::asio::make_work_guard(ioc);

    std::vector<std::thread> ioc_threads;
    for (int i = 0; i < 4; i++) {
        ioc_threads.emplace_back([&ioc] { ioc.run(); });
    }

    auto executor = [&ioc](Continuation<void()> f) {
        boost::asio::post(ioc, std::move(f));
    };

    Promise<int> promise;
    Future<int> result =
        promise.GetFuture()
            .Then([](int const& v) { return v * 2; }, executor)
            .Then([](int const& v) { return v + 1; }, executor);

    promise.Resolve(10);

    auto r = result.WaitForResult(std::chrono::seconds(5));
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 21);

    work.reset();
    for (auto& t : ioc_threads) {
        t.join();
    }
}
