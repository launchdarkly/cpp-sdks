#include <gtest/gtest.h>

#include <launchdarkly/client_side/data_source_status.hpp>

using launchdarkly::client_side::data_sources::DataSourceStatus;

TEST(DataSourceStatusTest, OstreamBasicStatus) {
    auto time =
        std::chrono::system_clock::time_point{std::chrono::milliseconds{0}};

    DataSourceStatus status(DataSourceStatus::DataSourceState::kInitializing,
                            time, std::nullopt);

    std::stringstream ss;
    ss << status;
    EXPECT_EQ("Status(INITIALIZING, Since(1970-01-01 00:00:00))", ss.str());
}

TEST(DataSourceStatusTest, OStreamErrorInfo) {
    auto time =
        std::chrono::system_clock::time_point{std::chrono::milliseconds{0}};

    DataSourceStatus status(
        DataSourceStatus::DataSourceState::kInterrupted, time,
        launchdarkly::common::data_sources::DataSourceStatusErrorInfo(
            launchdarkly::common::data_sources::DataSourceStatusErrorKind::
                kInvalidData,
            404, "Bad times", time));

    std::stringstream ss;
    ss << status;
    EXPECT_EQ(
        "Status(INTERRUPTED, Since(1970-01-01 00:00:00), Error(INVALID_DATA, "
        "Bad times, StatusCode(404), Since(1970-01-01 00:00:00)))",
        ss.str());
}
