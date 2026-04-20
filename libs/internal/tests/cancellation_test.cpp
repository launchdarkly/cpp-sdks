#include <gtest/gtest.h>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "launchdarkly/async/cancellation.hpp"

using namespace launchdarkly::async;

// A default-constructed token has no associated source, so a callback
// registered on it is never invoked.
TEST(Cancellation, DefaultToken_CallbackNeverInvoked) {
    CancellationToken token;

    bool invoked = false;
    CancellationCallback cb(token, [&invoked] { invoked = true; });

    EXPECT_FALSE(invoked);
    EXPECT_FALSE(token.IsCancelled());
}

// Cancelling before a callback is registered invokes the callback immediately
// inside the CancellationCallback constructor.
TEST(Cancellation, Cancel_BeforeRegistration_InvokesImmediately) {
    CancellationSource source;
    source.Cancel();

    bool invoked = false;
    CancellationCallback cb(source.GetToken(), [&invoked] { invoked = true; });

    EXPECT_TRUE(invoked);
}

// The normal case: cancel after registration invokes the callback.
TEST(Cancellation, Cancel_AfterRegistration_InvokesCallback) {
    CancellationSource source;

    bool invoked = false;
    CancellationCallback cb(source.GetToken(), [&invoked] { invoked = true; });

    EXPECT_FALSE(invoked);
    source.Cancel();
    EXPECT_TRUE(invoked);
}

// Cancelling more than once is a no-op: callbacks are invoked exactly once.
TEST(Cancellation, Cancel_Idempotent_CallbackInvokedOnce) {
    CancellationSource source;

    int count = 0;
    CancellationCallback cb(source.GetToken(), [&count] { count++; });

    source.Cancel();
    source.Cancel();
    source.Cancel();

    EXPECT_EQ(count, 1);
}

// Destroying a CancellationCallback before Cancel() deregisters it; the
// callback is not invoked when Cancel() is later called.
TEST(Cancellation, Deregister_BeforeCancel_CallbackNotInvoked) {
    CancellationSource source;

    bool invoked = false;
    {
        CancellationCallback cb(source.GetToken(),
                                [&invoked] { invoked = true; });
    }

    source.Cancel();

    EXPECT_FALSE(invoked);
}

// Destroying a CancellationCallback after Cancel() has already run does not
// crash or block; it is a no-op.
TEST(Cancellation, Deregister_AfterCancel_NoOp) {
    CancellationSource source;
    CancellationToken token = source.GetToken();

    auto cb = std::make_unique<CancellationCallback>(token, [] {});
    source.Cancel();
    cb.reset();  // should not crash or block
}

// All registered callbacks are invoked when Cancel() is called.
TEST(Cancellation, MultipleCallbacks_AllInvoked) {
    CancellationSource source;
    CancellationToken token = source.GetToken();

    int a = 0, b = 0, c = 0;
    CancellationCallback cb1(token, [&a] { a++; });
    CancellationCallback cb2(token, [&b] { b++; });
    CancellationCallback cb3(token, [&c] { c++; });

    source.Cancel();

    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 1);
    EXPECT_EQ(c, 1);
}

// Only callbacks that have not been deregistered are invoked.
TEST(Cancellation, PartialDeregister_OnlyActiveCallbacksInvoked) {
    CancellationSource source;
    CancellationToken token = source.GetToken();

    int a = 0, b = 0, c = 0;
    CancellationCallback cb1(token, [&a] { a++; });
    auto cb2 = std::make_unique<CancellationCallback>(token, [&b] { b++; });
    CancellationCallback cb3(token, [&c] { c++; });

    cb2.reset();
    source.Cancel();

    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 0);
    EXPECT_EQ(c, 1);
}

// IsCancelled reflects the state of the source correctly.
TEST(Cancellation, IsCancelled_BeforeAndAfterCancel) {
    CancellationSource source;
    CancellationToken token = source.GetToken();

    EXPECT_FALSE(source.IsCancelled());
    EXPECT_FALSE(token.IsCancelled());

    source.Cancel();

    EXPECT_TRUE(source.IsCancelled());
    EXPECT_TRUE(token.IsCancelled());
}

// Copies of a CancellationSource share the same underlying state.
TEST(Cancellation, SourceCopyable_CopiesShareState) {
    CancellationSource source1;
    CancellationSource source2 = source1;

    bool invoked = false;
    CancellationCallback cb(source1.GetToken(), [&invoked] { invoked = true; });

    source2.Cancel();

    EXPECT_TRUE(invoked);
    EXPECT_TRUE(source1.IsCancelled());
}

// Copies of a CancellationToken share the same underlying state.
TEST(Cancellation, TokenCopyable_CopiesShareState) {
    CancellationSource source;
    CancellationToken token1 = source.GetToken();
    CancellationToken token2 = token1;

    bool invoked = false;
    CancellationCallback cb(token2, [&invoked] { invoked = true; });

    source.Cancel();

    EXPECT_TRUE(invoked);
    EXPECT_TRUE(token1.IsCancelled());
}

// If a CancellationCallback is destroyed while its callback is executing on
// another thread, the destructor blocks until execution completes. This
// guarantees that anything captured by the callback remains valid for its
// entire execution.
TEST(Cancellation, DestructorBlocks_WhileCallbackExecuting) {
    CancellationSource source;

    std::mutex mtx;
    std::condition_variable cv;
    bool callback_started = false;
    bool callback_completed = false;

    auto cb = std::make_unique<CancellationCallback>(source.GetToken(), [&] {
        {
            std::lock_guard lk(mtx);
            callback_started = true;
        }
        cv.notify_one();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        callback_completed = true;
    });

    std::thread t([&source] { source.Cancel(); });

    {
        std::unique_lock lk(mtx);
        cv.wait(lk, [&] { return callback_started; });
    }

    // Destroy cb while the callback is sleeping on thread t. The destructor
    // should block until the callback sets callback_completed.
    cb.reset();

    EXPECT_TRUE(callback_completed);

    t.join();
}

// Destroying a CancellationCallback from within another callback on the same
// thread does not deadlock. This covers the re-entrant path in Deregister.
TEST(Cancellation, SameThreadDeregister_NoDeadlock) {
    CancellationSource source;
    CancellationToken token = source.GetToken();

    std::unique_ptr<CancellationCallback> cb2;
    CancellationCallback cb1(token, [&cb2] { cb2.reset(); });
    cb2 = std::make_unique<CancellationCallback>(token, [] {});

    source.Cancel();  // cb1 fires, destroys cb2 from the same thread
}
