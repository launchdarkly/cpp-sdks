#include <gtest/gtest.h>

#include <numeric>
#include <thread>

#include "backoff.hpp"
#include "backoff_detail.hpp"

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

class BackoffOverflowEndToEndTest
    : public ::testing::TestWithParam<std::uint64_t> {};

TEST_P(BackoffOverflowEndToEndTest, BackoffDoesNotOverflowWithVariousAttempts) {
    Backoff backoff(
        std::chrono::milliseconds{1000}, std::chrono::milliseconds{30000}, 0,
        std::chrono::milliseconds{60000}, [](auto ratio) { return 0; });

    std::uint64_t const attempts = GetParam();

    for (int i = 0; i < attempts; i++) {
        backoff.fail();
    }

    auto const val = backoff.delay();
    EXPECT_EQ(val, std::chrono::milliseconds{30000});
}

INSTANTIATE_TEST_SUITE_P(VariousBackoffAttempts,
                         BackoffOverflowEndToEndTest,
                         ::testing::Values<std::uint64_t>(63, 64, 65, 1000));

class BackoffOverflowUnitTest : public ::testing::TestWithParam<std::uint64_t> {
};

TEST_P(BackoffOverflowUnitTest,
       BackoffDoesNotOverflowWithVariousAttemptsInCalculateBackoff) {
    std::uint64_t const attempts = GetParam();

    constexpr std::uint64_t max_exponent = 5;

    // Given an exponent of 5, and initial delay of 1ms, the max should be
    // 2^5 = 32 for any amount of attempts > 5.

    constexpr auto initial = std::chrono::milliseconds{1};
    constexpr auto max = std::chrono::milliseconds{32};

    auto const milliseconds = launchdarkly::sse::detail::calculate_backoff(
        initial, attempts, max_exponent);

    ASSERT_EQ(milliseconds, max);
}

TEST_P(BackoffOverflowUnitTest,
       BackoffDoesNotOverflowWithVariousAttemptsInDelay) {
    std::uint64_t const attempts = GetParam();

    constexpr std::uint64_t max_exponent = 5;

    // Given an exponent of 5, and initial delay of 1ms, the max should be
    // 2^5 = 32 for any amount of attempts > 5.

    constexpr auto initial = std::chrono::milliseconds{1};
    constexpr auto max = std::chrono::milliseconds{32};

    auto const milliseconds = launchdarkly::sse::detail::delay(
        initial, max, attempts, max_exponent, 0, [](auto ratio) { return 0; });

    ASSERT_EQ(milliseconds, max);
}

INSTANTIATE_TEST_SUITE_P(VariousBackoffAttempts,
                         BackoffOverflowUnitTest,
                         ::testing::Values<std::uint64_t>(
                             1000,
                             10000,
                             100000,
                             std::numeric_limits<std::uint64_t>::max()));
