#include <gtest/gtest.h>

#include <launchdarkly/data_sources/data_source_status_base.hpp>
#include "data_source_status_error_kind.hpp"

namespace test_things {
enum class TestDataSourceStates { kStateA = 0, kStateB = 1, kStateC = 2 };

class DataSourceStatus
    : public ::launchdarkly::common::data_sources::DataSourceStatusBase<
          TestDataSourceStates> {
   public:
    using DateTime = std::chrono::time_point<std::chrono::system_clock>;
    using DataSourceState = TestDataSourceStates;

    using ErrorInfo =
        ::launchdarkly::common::data_sources::DataSourceStatusBase<
            TestDataSourceStates>::ErrorInfo;

    DataSourceStatus(DataSourceState state,
                     DateTime state_since,
                     std::optional<ErrorInfo> last_error)
        : ::launchdarkly::common::data_sources::DataSourceStatusBase<
              TestDataSourceStates>(state, state_since, last_error) {}

    DataSourceStatus(DataSourceStatus const& status)
        : ::launchdarkly::common::data_sources::DataSourceStatusBase<
              TestDataSourceStates>(status) {}
};

std::ostream& operator<<(std::ostream& out, TestDataSourceStates const& state) {
    switch (state) {
        case TestDataSourceStates::kStateA:
            out << "kStateA";
            break;
        case TestDataSourceStates::kStateB:
            out << "kStateB";
            break;
        case TestDataSourceStates::kStateC:
            out << "kStateC";
            break;
    }

    return out;
}

std::ostream& operator<<(std::ostream& out, DataSourceStatus const& status) {
    std::time_t as_time_t =
        std::chrono::system_clock::to_time_t(status.StateSince());
    out << "Status(" << status.State() << ", Since("
        << std::put_time(std::gmtime(&as_time_t), "%Y-%m-%d %H:%M:%S") << ")";
    if (status.LastError()) {
        out << ", " << status.LastError().value();
    }
    out << ")";
    return out;
}
}  // namespace test_things

TEST(DataSourceStatusTest, OstreamBasicStatus) {
    auto time =
        std::chrono::system_clock::time_point{std::chrono::milliseconds{0}};
    ;
    test_things::DataSourceStatus status(
        test_things::DataSourceStatus::DataSourceState::kStateA, time,
        std::nullopt);

    std::stringstream ss;
    ss << status;
    ss.flush();
    EXPECT_EQ("Status(kStateA, Since(1970-01-01 00:00:00))", ss.str());
}

TEST(DataSourceStatusTest, OStreamErrorInfo) {
    auto time =
        std::chrono::system_clock::time_point{std::chrono::milliseconds{0}};
    ;
    test_things::DataSourceStatus status(
        test_things::DataSourceStatus::DataSourceState::kStateC, time,
        launchdarkly::common::data_sources::DataSourceStatusErrorInfo(
            launchdarkly::common::data_sources::DataSourceStatusErrorKind::
                kInvalidData,
            404, "Bad times", time));

    std::stringstream ss;
    ss << status;
    ss.flush();
    EXPECT_EQ(
        "Status(kStateC, Since(1970-01-01 00:00:00), Error(INVALID_DATA, Bad "
        "times, StatusCode(404), Since(1970-01-01 00:00:00)))",
        ss.str());
}