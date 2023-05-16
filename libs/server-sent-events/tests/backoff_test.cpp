#include <gtest/gtest.h>

#include <thread>

#include "backoff.hpp"

using launchdarkly::sse::Backoff;

// Demonstrate some basic assertions.
TEST(BackoffTests, DelayStartsAtInitial) {
    Backoff backoff(
        std::chrono::milliseconds{1000}, std::chrono::milliseconds{30000}, 0,
        std::chrono::milliseconds{60000}, [](auto ratio) { return 0; });

    EXPECT_EQ(std::chrono::milliseconds{1000}, backoff.delay());
}

TEST(BackoffTests, ExponentialBackoffConsecutiveFailures) {
    Backoff backoff(
        std::chrono::milliseconds{1000}, std::chrono::milliseconds{30000}, 0,
        std::chrono::milliseconds{60000}, [](auto ratio) { return 0; });

    EXPECT_EQ(1000, backoff.delay().count());
    backoff.fail();

    EXPECT_EQ(2000, backoff.delay().count());
    backoff.fail();

    EXPECT_EQ(4000, backoff.delay().count());
}

TEST(BackoffTests, RespectsMax) {
    Backoff backoff(
        std::chrono::milliseconds{1000}, std::chrono::milliseconds{2000}, 0,
        std::chrono::milliseconds{60000}, [](auto ratio) { return 0; });

    EXPECT_EQ(1000, backoff.delay().count());
    backoff.fail();

    EXPECT_EQ(2000, backoff.delay().count());
    backoff.fail();

    EXPECT_EQ(2000, backoff.delay().count());
}

TEST(BackoffTests, JittersBackoffValue) {
    Backoff backoff(
        std::chrono::milliseconds{1000}, std::chrono::milliseconds{30000}, 0,
        std::chrono::milliseconds{60000}, [](auto ratio) { return 0.25; });

    // Our "random" method is going to always return 0.25, so we can see
    // what the jitter impact is.

    EXPECT_EQ(750, backoff.delay().count());
    backoff.fail();

    EXPECT_EQ(1500, backoff.delay().count());
    backoff.fail();

    EXPECT_EQ(3000, backoff.delay().count());
}

TEST(BackoffTests, BackoffResetsAfterResetInterval) {
    // In this test we set the reset interval short, then sleep to
    // check that it does reset. As short as this interval is, and the
    // sleep being double, it should be consistent.
    Backoff backoff(
        std::chrono::milliseconds{1000}, std::chrono::milliseconds{30000}, 0,
        std::chrono::milliseconds{50}, [](auto ratio) { return 0; });

    backoff.fail();     // 2000;
    backoff.fail();     // 4000;
    backoff.succeed();  // Still 4000;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    backoff.fail();  // Will be first failure, so 1000;

    EXPECT_EQ(1000, backoff.delay().count());
}

TEST(BackoffTests, BackoffDoesNotResetActiveConnectionWasTooShort) {
    Backoff backoff(
        std::chrono::milliseconds{1000}, std::chrono::milliseconds{30000}, 0,
        std::chrono::milliseconds{6000}, [](auto ratio) { return 0; });

    backoff.fail();     // 2000;
    backoff.fail();     // 4000;
    backoff.succeed();  // Still 4000;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    backoff.fail();  // Connection was not active for long enough, double.

    EXPECT_EQ(8000, backoff.delay().count());
}
