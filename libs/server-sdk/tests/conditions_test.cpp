#include <gtest/gtest.h>

#include <data_systems/fdv2/conditions.hpp>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>

#include <chrono>
#include <thread>

using namespace launchdarkly::server_side::data_interfaces;
using namespace launchdarkly::server_side::data_systems;
using namespace std::chrono_literals;

using launchdarkly::async::CancellationToken;

namespace {

// Holds an io_context running on a worker thread; the executor produced by
// GetExecutor() is what the conditions schedule timer work on. Using a real
// running executor exercises the full async::Delay path including timer
// cancellation and thread-handoff of the resolution callback.
class RunningIoContext {
   public:
    RunningIoContext()
        : work_guard_(boost::asio::make_work_guard(ioc_)),
          thread_([this] { ioc_.run(); }) {}

    ~RunningIoContext() {
        work_guard_.reset();
        ioc_.stop();
        thread_.join();
    }

    boost::asio::any_io_executor GetExecutor() { return ioc_.get_executor(); }

   private:
    boost::asio::io_context ioc_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work_guard_;
    std::thread thread_;
};

}  // namespace

// ============================================================================
// FallbackCondition
// ============================================================================

TEST(FallbackConditionTest, InterruptedArmsTimerWhichFiresAfterTimeout) {
    RunningIoContext ioc;
    FallbackCondition condition(ioc.GetExecutor(), /*timeout=*/100ms);
    auto future = condition.Execute();

    condition.Inform(FDv2SourceResult{FDv2SourceResult::Interrupted{
        FDv2SourceResult::ErrorInfo{
            FDv2SourceResult::ErrorInfo::ErrorKind::kNetworkError,
            /*status_code=*/0, "boom", std::chrono::system_clock::now()},
    }});

    auto result = future.WaitForResult(1s);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(IFDv2Condition::Type::kFallback, *result);
}

TEST(FallbackConditionTest, ChangeSetCancelsActiveTimer) {
    RunningIoContext ioc;
    FallbackCondition condition(ioc.GetExecutor(), /*timeout=*/100ms);
    auto future = condition.Execute();

    // Arm the timer with Interrupted, then cancel via ChangeSet before it
    // fires.
    condition.Inform(FDv2SourceResult{FDv2SourceResult::Interrupted{
        FDv2SourceResult::ErrorInfo{
            FDv2SourceResult::ErrorInfo::ErrorKind::kNetworkError,
            /*status_code=*/0, "boom", std::chrono::system_clock::now()},
    }});
    condition.Inform(FDv2SourceResult{FDv2SourceResult::ChangeSet{
        launchdarkly::data_model::ChangeSet<ChangeSetData>{
            launchdarkly::data_model::ChangeSetType::kFull,
            {},
            launchdarkly::data_model::Selector{},
        },
    }});

    // Wait well past the 100ms threshold; future should remain unresolved.
    std::this_thread::sleep_for(300ms);
    EXPECT_FALSE(future.IsFinished());
}

TEST(FallbackConditionTest, CloseCancelsActiveTimerAndResolvesWithCancelled) {
    RunningIoContext ioc;
    FallbackCondition condition(ioc.GetExecutor(), /*timeout=*/100ms);
    auto future = condition.Execute();

    condition.Inform(FDv2SourceResult{FDv2SourceResult::Interrupted{
        FDv2SourceResult::ErrorInfo{
            FDv2SourceResult::ErrorInfo::ErrorKind::kNetworkError,
            /*status_code=*/0, "boom", std::chrono::system_clock::now()},
    }});
    condition.Close();

    auto result = future.WaitForResult(200ms);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(IFDv2Condition::Type::kCancelled, *result);
}

// ============================================================================
// RecoveryCondition
// ============================================================================

TEST(RecoveryConditionTest, TimerArmedAtConstructionFiresAfterTimeout) {
    RunningIoContext ioc;
    RecoveryCondition condition(ioc.GetExecutor(), /*timeout=*/100ms);

    auto result = condition.Execute().WaitForResult(1s);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(IFDv2Condition::Type::kRecovery, *result);
}

TEST(RecoveryConditionTest, InformDoesNotAffectTimer) {
    RunningIoContext ioc;
    RecoveryCondition condition(ioc.GetExecutor(), /*timeout=*/100ms);
    auto future = condition.Execute();

    // Recovery is purely time-based; results from the synchronizer should not
    // disturb the timer in either direction.
    condition.Inform(FDv2SourceResult{FDv2SourceResult::Interrupted{
        FDv2SourceResult::ErrorInfo{
            FDv2SourceResult::ErrorInfo::ErrorKind::kNetworkError,
            /*status_code=*/0, "boom", std::chrono::system_clock::now()},
    }});
    condition.Inform(FDv2SourceResult{FDv2SourceResult::ChangeSet{
        launchdarkly::data_model::ChangeSet<ChangeSetData>{
            launchdarkly::data_model::ChangeSetType::kFull,
            {},
            launchdarkly::data_model::Selector{},
        },
    }});

    auto result = future.WaitForResult(1s);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(IFDv2Condition::Type::kRecovery, *result);
}

TEST(RecoveryConditionTest, CloseCancelsActiveTimerAndResolvesWithCancelled) {
    RunningIoContext ioc;
    RecoveryCondition condition(ioc.GetExecutor(), /*timeout=*/100ms);
    auto future = condition.Execute();

    condition.Close();

    auto result = future.WaitForResult(200ms);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(IFDv2Condition::Type::kCancelled, *result);
}

// ============================================================================
// Conditions
// ============================================================================

TEST(ConditionsTest, EmptyAggregateNeverResolves) {
    Conditions conditions({});

    auto result = conditions.GetFuture(CancellationToken{}).WaitForResult(50ms);

    EXPECT_FALSE(result.has_value());
}

TEST(ConditionsTest, AggregateResolvesWithTypeOfFirstFiringCondition) {
    RunningIoContext ioc;

    // Recovery's timer is much shorter, so it should win the race.
    std::vector<std::unique_ptr<IFDv2Condition>> conds;
    conds.push_back(
        std::make_unique<FallbackCondition>(ioc.GetExecutor(), /*timeout=*/1s));
    conds.push_back(std::make_unique<RecoveryCondition>(ioc.GetExecutor(),
                                                        /*timeout=*/100ms));
    Conditions conditions(std::move(conds));

    auto result = conditions.GetFuture(CancellationToken{}).WaitForResult(1s);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(IFDv2Condition::Type::kRecovery, *result);
}

TEST(ConditionsTest, InformForwardsToAllUnderlyingConditions) {
    RunningIoContext ioc;

    // Fallback's timer is shorter than recovery's; informing Interrupted arms
    // the fallback timer, which will then beat recovery.
    std::vector<std::unique_ptr<IFDv2Condition>> conds;
    conds.push_back(std::make_unique<FallbackCondition>(ioc.GetExecutor(),
                                                        /*timeout=*/100ms));
    conds.push_back(
        std::make_unique<RecoveryCondition>(ioc.GetExecutor(), /*timeout=*/1s));
    Conditions conditions(std::move(conds));

    conditions.Inform(FDv2SourceResult{FDv2SourceResult::Interrupted{
        FDv2SourceResult::ErrorInfo{
            FDv2SourceResult::ErrorInfo::ErrorKind::kNetworkError,
            /*status_code=*/0, "boom", std::chrono::system_clock::now()},
    }});

    auto result = conditions.GetFuture(CancellationToken{}).WaitForResult(1s);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(IFDv2Condition::Type::kFallback, *result);
}

TEST(ConditionsTest, CloseForwardsToAllUnderlyingConditions) {
    RunningIoContext ioc;

    std::vector<std::unique_ptr<IFDv2Condition>> conds;
    conds.push_back(std::make_unique<RecoveryCondition>(ioc.GetExecutor(),
                                                        /*timeout=*/100ms));
    conds.push_back(std::make_unique<RecoveryCondition>(ioc.GetExecutor(),
                                                        /*timeout=*/100ms));
    Conditions conditions(std::move(conds));

    conditions.Close();

    auto result =
        conditions.GetFuture(CancellationToken{}).WaitForResult(200ms);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(IFDv2Condition::Type::kCancelled, *result);
}
