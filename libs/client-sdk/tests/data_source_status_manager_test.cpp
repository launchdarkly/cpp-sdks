#include <gtest/gtest.h>

#include <launchdarkly/client_side/data_sources/detail/data_source_status_manager.hpp>

using launchdarkly::client_side::data_sources::DataSourceStatus;
using launchdarkly::client_side::data_sources::IDataSourceStatusProvider;
using launchdarkly::client_side::data_sources::DataSourceStatusManager;

class DataSourceStateParameterizedTestFixture
    : public ::testing::TestWithParam<DataSourceStatus::DataSourceState> {};

TEST(DataSourceStatusManagerTests,
     WhenInitializingInterruptedDoesNotChangeState) {
    DataSourceStatusManager status_manager;

    status_manager.SetState(DataSourceStatus::DataSourceState::kInterrupted);

    EXPECT_EQ(DataSourceStatus::DataSourceState::kInitializing,
              status_manager.Status().State());
}

TEST(DataSourceStatusManagerTests,
     WhenInitializingInterruptedDoesNotProduceEvent) {
    DataSourceStatusManager status_manager;

    std::atomic<bool> got_event(false);
    status_manager.OnDataSourceStatusChange(
        [&got_event](auto status) { got_event.store(true); });

    status_manager.SetState(DataSourceStatus::DataSourceState::kInterrupted);

    EXPECT_FALSE(got_event);
}

TEST(DataSourceStatusManagerTests, CanTransitionToValidFromInitializing) {
    DataSourceStatusManager status_manager;

    status_manager.SetState(DataSourceStatus::DataSourceState::kValid);

    EXPECT_EQ(DataSourceStatus::DataSourceState::kValid,
              status_manager.Status().State());
}

TEST(DataSourceStatusManagerTests, CanTransitionFromValidToInterrupted) {
    DataSourceStatusManager status_manager;

    status_manager.SetState(DataSourceStatus::DataSourceState::kValid);

    status_manager.SetState(DataSourceStatus::DataSourceState::kInterrupted);

    EXPECT_EQ(DataSourceStatus::DataSourceState::kInterrupted,
              status_manager.Status().State());
}

TEST(DataSourceStatusManagerTests, CanTransitionToShutdownFromInitializing) {
    DataSourceStatusManager status_manager;

    status_manager.SetState(DataSourceStatus::DataSourceState::kShutdown);

    EXPECT_EQ(DataSourceStatus::DataSourceState::kShutdown,
              status_manager.Status().State());
}

TEST_P(DataSourceStateParameterizedTestFixture, SameStateProducesNoEvent) {
    DataSourceStatusManager status_manager;

    status_manager.SetState(GetParam());

    // We start in initializing, so it doesn't produce an event.
    std::atomic<bool> got_event(false);
    status_manager.OnDataSourceStatusChange([&got_event](auto status) {
        got_event.store(true);
        EXPECT_EQ(GetParam(), status.State());
    });

    status_manager.SetState(GetParam());

    EXPECT_FALSE(got_event);
}

INSTANTIATE_TEST_SUITE_P(
    DataSourceStatusManagerTests,
    DataSourceStateParameterizedTestFixture,
    testing::Values(DataSourceStatus::DataSourceState::kSetOffline,
                    DataSourceStatus::DataSourceState::kShutdown,
                    DataSourceStatus::DataSourceState::kValid,
                    DataSourceStatus::DataSourceState::kInitializing,
                    DataSourceStatus::DataSourceState::kInterrupted));

class DataSourceErrorKindParameterizedTestFixture
    : public ::testing::TestWithParam<DataSourceStatus::ErrorInfo::ErrorKind> {
};

TEST_P(DataSourceErrorKindParameterizedTestFixture, HandlesSettingErrorKind) {
    DataSourceStatusManager status_manager;

    status_manager.SetError(GetParam(), "An error message");

    EXPECT_EQ(GetParam(), status_manager.Status().LastError()->Kind());
    EXPECT_EQ("An error message",
              status_manager.Status().LastError()->Message());
}

TEST_P(DataSourceErrorKindParameterizedTestFixture, ProducesEventOnError) {
    DataSourceStatusManager status_manager;

    std::atomic<bool> got_event(false);
    status_manager.OnDataSourceStatusChange([&got_event](auto status) {
        got_event.store(true);
        EXPECT_EQ(GetParam(), status.LastError()->Kind());
    });

    status_manager.SetError(GetParam(), "An error message");

    EXPECT_TRUE(got_event);
}

INSTANTIATE_TEST_SUITE_P(
    DataSourceStatusManagerTests,
    DataSourceErrorKindParameterizedTestFixture,
    testing::Values(DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
                    DataSourceStatus::ErrorInfo::ErrorKind::kErrorResponse,
                    DataSourceStatus::ErrorInfo::ErrorKind::kNetworkError,
                    DataSourceStatus::ErrorInfo::ErrorKind::kUnknown));

TEST(DataSourceStatusManagerTests, CanSetErrorViaStatusCode) {
    DataSourceStatusManager status_manager;

    status_manager.SetError(404, "Bad times");
    EXPECT_EQ(DataSourceStatus::ErrorInfo::ErrorKind::kErrorResponse,
              status_manager.Status().LastError()->Kind());
    EXPECT_EQ(404, status_manager.Status().LastError()->StatusCode());

    EXPECT_EQ("Bad times", status_manager.Status().LastError()->Message());
}

TEST(DataSourceStatusManagerTests, NoErrorIfNoErrorHasHappened) {
    DataSourceStatusManager status_manager;

    EXPECT_FALSE(status_manager.Status().LastError().has_value());
}

TEST(DataSourceStatusManagerTests, TimeIsUpdatedOnStateChange) {
    DataSourceStatusManager status_manager;
    status_manager.SetState(DataSourceStatus::DataSourceState::kValid);

    auto initial = status_manager.Status().StateSince();
    status_manager.SetState(DataSourceStatus::DataSourceState::kInterrupted);

    EXPECT_NE(initial.time_since_epoch().count(),
              status_manager.Status().StateSince().time_since_epoch().count());
}

TEST(DataSourceStatusManagerTests, SameStateDoesNotUpdateTime) {
    DataSourceStatusManager status_manager;
    status_manager.SetState(DataSourceStatus::DataSourceState::kValid);

    auto initial = status_manager.Status().StateSince();
    status_manager.SetState(DataSourceStatus::DataSourceState::kValid);

    EXPECT_EQ(initial.time_since_epoch().count(),
              status_manager.Status().StateSince().time_since_epoch().count());
}

TEST(DataSourceStatusManagerTests, ErrorHasTimeStamp) {
    DataSourceStatusManager status_manager;
    auto before_error_set = std::chrono::system_clock::now();

    status_manager.SetError(404, "Bad news");

    auto after_error_set = std::chrono::system_clock::now();

    EXPECT_EQ(status_manager.Status().LastError()->Time(),
              status_manager.Status().LastError()->Time());

    EXPECT_LE(
        before_error_set.time_since_epoch().count(),
        status_manager.Status().LastError()->Time().time_since_epoch().count());
    EXPECT_GE(
        after_error_set.time_since_epoch().count(),
        status_manager.Status().LastError()->Time().time_since_epoch().count());
}
