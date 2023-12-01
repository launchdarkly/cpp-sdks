#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "data_systems/lazy_load/lazy_load_system.hpp"

#include <set>
#include <unordered_map>

#include "data_components/serialization_adapters/json_deserializer.hpp"
#include "spy_logger.hpp"

using namespace launchdarkly;
using namespace launchdarkly::server_side;
using namespace launchdarkly::server_side::config;

using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

class MockDataReader : public integrations::ISerializedDataReader {
   public:
    MOCK_METHOD(GetResult,
                Get,
                (integrations::ISerializedItemKind const& kind,
                 std::string const& itemKey),
                (override, const));
    MOCK_METHOD(AllResult,
                All,
                (integrations::ISerializedItemKind const& kind),
                (override, const));
    MOCK_METHOD(std::string const&, Identity, (), (override, const));
    MOCK_METHOD(bool, Initialized, (), (override, const));
    explicit MockDataReader(std::string name) : name_(std::move(name)) {
        ON_CALL(*this, Identity).WillByDefault(::testing::ReturnRef(name_));
    }

   private:
    std::string const name_;
};

class LazyLoadTest : public ::testing::Test {
   public:
    std::string mock_reader_name = "fake reader";
    std::shared_ptr<NiceMock<MockDataReader>> mock_reader;
    std::shared_ptr<logging::SpyLoggerBackend> spy_logger_backend;
    Logger const logger;
    data_components::DataSourceStatusManager status_manager;
    LazyLoadTest()
        : mock_reader(
              std::make_shared<NiceMock<MockDataReader>>(mock_reader_name)),
          spy_logger_backend(std::make_shared<logging::SpyLoggerBackend>()),
          logger(spy_logger_backend) {}
};

TEST_F(LazyLoadTest, IdentityWrapsReaderIdentity) {
    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled,
        std::chrono::milliseconds(100), mock_reader};

    data_systems::LazyLoad const lazy_load(logger, config, status_manager);

    ASSERT_EQ(lazy_load.Identity(),
              "lazy load via " + mock_reader_name + " (JSON)");
}

TEST_F(LazyLoadTest, ReaderIsNotQueriedRepeatedlyIfFlagIsCached) {
    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled,
        std::chrono::seconds(10), mock_reader};

    data_systems::LazyLoad const lazy_load(logger, config, status_manager);

    EXPECT_CALL(*mock_reader, Get(testing::_, "foo"))
        .WillOnce(Return(integrations::SerializedItemDescriptor{
            1, false, "{\"key\":\"foo\",\"version\":1}"}));

    for (std::size_t i = 0; i < 20; i++) {
        ASSERT_TRUE(lazy_load.GetFlag("foo"));
    }
}

TEST_F(LazyLoadTest, ReaderIsNotQueriedRepeatedlyIfSegmentIsCached) {
    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled,
        std::chrono::seconds(10), mock_reader};

    data_systems::LazyLoad const lazy_load(logger, config, status_manager);

    EXPECT_CALL(*mock_reader, Get(testing::_, "foo"))
        .WillOnce(Return(integrations::SerializedItemDescriptor{
            1, false, "{\"key\":\"foo\",\"version\":1}"}));

    for (std::size_t i = 0; i < 20; i++) {
        ASSERT_TRUE(lazy_load.GetSegment("foo"));
    }
}

TEST_F(LazyLoadTest, ReaderIsNotQueriedRepeatedlyIfFlagCannotBeFetched) {
    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled,
        std::chrono::seconds(10), mock_reader};

    data_systems::LazyLoad const lazy_load(logger, config, status_manager);

    EXPECT_CALL(*mock_reader, Get(testing::_, "foo"))
        .WillOnce(Return(tl::make_unexpected(
            integrations::ISerializedDataReader::Error{"oops"})));

    for (std::size_t i = 0; i < 20; i++) {
        ASSERT_FALSE(lazy_load.GetFlag("foo"));
    };

    ASSERT_TRUE(spy_logger_backend->Count(21));  // 20 debug logs + 1 error log
    ASSERT_TRUE(spy_logger_backend->Contains(1, LogLevel::kError, "oops"));
}

TEST_F(LazyLoadTest, ReaderIsNotQueriedRepeatedlyIfSegmentCannotBeFetched) {
    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled,
        std::chrono::seconds(10), mock_reader};

    data_systems::LazyLoad const lazy_load(logger, config, status_manager);

    EXPECT_CALL(*mock_reader, Get(testing::_, "foo"))
        .WillOnce(Return(tl::make_unexpected(
            integrations::ISerializedDataReader::Error{"oops"})));

    for (std::size_t i = 0; i < 20; i++) {
        ASSERT_FALSE(lazy_load.GetSegment("foo"));
    };

    ASSERT_TRUE(spy_logger_backend->Count(21));
    ASSERT_TRUE(spy_logger_backend->Contains(1, LogLevel::kError, "oops"));
}

TEST_F(LazyLoadTest, RefreshesFlagIfStale) {
    using TimePoint = data_systems::LazyLoad::ClockType::time_point;

    constexpr auto refresh_ttl = std::chrono::seconds(10);

    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled, refresh_ttl,
        mock_reader};

    TimePoint now{std::chrono::seconds(0)};

    data_systems::LazyLoad const lazy_load(logger, config, status_manager,
                                           [&]() { return now; });

    {
        InSequence s;
        EXPECT_CALL(*mock_reader, Get(testing::_, "foo"))
            .WillOnce(Return(integrations::SerializedItemDescriptor{
                1, false, "{\"key\":\"foo\",\"version\":1}"}));
        EXPECT_CALL(*mock_reader, Get(testing::_, "foo"))
            .WillOnce(Return(integrations::SerializedItemDescriptor{
                2, false, "{\"key\":\"foo\",\"version\":2}"}));
    }

    for (std::size_t i = 0; i < 10; i++) {
        auto flag = lazy_load.GetFlag("foo");
        ASSERT_TRUE(flag);
        ASSERT_EQ(flag->version, 1);
    }

    now = TimePoint{refresh_ttl + std::chrono::seconds(1)};

    for (std::size_t i = 0; i < 10; i++) {
        auto flag = lazy_load.GetFlag("foo");
        ASSERT_TRUE(flag);
        ASSERT_EQ(flag->version, 2);
    }
}

TEST_F(LazyLoadTest, RefreshesSegmentIfStale) {
    using TimePoint = data_systems::LazyLoad::ClockType::time_point;

    constexpr auto refresh_ttl = std::chrono::seconds(10);

    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled, refresh_ttl,
        mock_reader};

    TimePoint now{std::chrono::seconds(0)};

    data_systems::LazyLoad const lazy_load(logger, config, status_manager,
                                           [&]() { return now; });

    {
        InSequence s;
        EXPECT_CALL(*mock_reader, Get(testing::_, "foo"))
            .WillOnce(Return(integrations::SerializedItemDescriptor{
                1, false, "{\"key\":\"foo\",\"version\":1}"}));
        EXPECT_CALL(*mock_reader, Get(testing::_, "foo"))
            .WillOnce(Return(integrations::SerializedItemDescriptor{
                2, false, "{\"key\":\"foo\",\"version\":2}"}));
    }

    for (std::size_t i = 0; i < 10; i++) {
        auto segment = lazy_load.GetSegment("foo");
        ASSERT_TRUE(segment);
        ASSERT_EQ(segment->version, 1);
    }

    now = TimePoint{refresh_ttl + std::chrono::seconds(1)};

    for (std::size_t i = 0; i < 10; i++) {
        auto segment = lazy_load.GetSegment("foo");
        ASSERT_TRUE(segment);
        ASSERT_EQ(segment->version, 2);
    }
}

TEST_F(LazyLoadTest, AllFlagsRefreshesIndividualFlag) {
    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled,
        std::chrono::seconds(10), mock_reader};

    // We want to demonstrate that an individual flag will be
    // refreshed not just when we grab that single flag, but also if
    // we call AllFlags.

    {
        InSequence s;
        EXPECT_CALL(*mock_reader, Get(testing::_, "foo"))
            .WillOnce(Return(integrations::SerializedItemDescriptor{
                1, false, "{\"key\":\"foo\",\"version\":1}"}));
        EXPECT_CALL(*mock_reader, All(testing::_))
            .WillOnce(Return(
                std::unordered_map<std::string,
                                   integrations::SerializedItemDescriptor>{
                    {"foo", {2, false, "{\"key\":\"foo\",\"version\":2}"}}}));
    }

    data_systems::LazyLoad const lazy_load(logger, config, status_manager);

    auto const flag1 = lazy_load.GetFlag("foo");
    ASSERT_TRUE(flag1);
    ASSERT_EQ(flag1->version, 1);

    auto const all_flags = lazy_load.AllFlags();
    ASSERT_EQ(all_flags.size(), 1);
    ASSERT_EQ(all_flags.at("foo")->version, 2);

    auto const flag2 = lazy_load.GetFlag("foo");
    ASSERT_TRUE(flag2);
    ASSERT_EQ(flag2->version, 2);
}

TEST_F(LazyLoadTest, AllSegmentsRefreshesIndividualSegment) {
    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled,
        std::chrono::seconds(10), mock_reader};

    // We want to demonstrate that an individual segment will be
    // refreshed not just when we grab that single segment, but also if
    // we call AllSegments.

    {
        InSequence s;
        EXPECT_CALL(*mock_reader, Get(testing::_, "foo"))
            .WillOnce(Return(integrations::SerializedItemDescriptor{
                1, false, "{\"key\":\"foo\",\"version\":1}"}));
        EXPECT_CALL(*mock_reader, All(testing::_))
            .WillOnce(Return(
                std::unordered_map<std::string,
                                   integrations::SerializedItemDescriptor>{
                    {"foo", {2, false, "{\"key\":\"foo\",\"version\":2}"}}}));
    }

    data_systems::LazyLoad const lazy_load(logger, config, status_manager);

    auto const segment1 = lazy_load.GetSegment("foo");
    ASSERT_TRUE(segment1);
    ASSERT_EQ(segment1->version, 1);

    auto const all_segments = lazy_load.AllSegments();
    ASSERT_EQ(all_segments.size(), 1);
    ASSERT_EQ(all_segments.at("foo")->version, 2);

    auto const segment2 = lazy_load.GetSegment("foo");
    ASSERT_TRUE(segment2);
    ASSERT_EQ(segment2->version, 2);
}

TEST_F(LazyLoadTest, InitializeNotQueriedRepeatedly) {
    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled,
        std::chrono::seconds(10), mock_reader};

    EXPECT_CALL(*mock_reader, Initialized).WillOnce(Return(false));

    data_systems::LazyLoad const lazy_load(logger, config, status_manager);

    for (std::size_t i = 0; i < 10; i++) {
        ASSERT_FALSE(lazy_load.Initialized());
    }
}

TEST_F(LazyLoadTest, InitializeCalledOnceThenNeverAgainAfterReturningTrue) {
    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled,
        std::chrono::seconds(10), mock_reader};

    EXPECT_CALL(*mock_reader, Initialized).WillOnce(Return(true));

    data_systems::LazyLoad const lazy_load(logger, config, status_manager);

    for (std::size_t i = 0; i < 10; i++) {
        ASSERT_TRUE(lazy_load.Initialized());
    }
}

TEST_F(LazyLoadTest, InitializeCalledAgainAfterTTL) {
    using TimePoint = data_systems::LazyLoad::ClockType::time_point;
    constexpr auto refresh_ttl = std::chrono::seconds(10);

    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled, refresh_ttl,
        mock_reader};

    {
        InSequence s;
        EXPECT_CALL(*mock_reader, Initialized).WillOnce(Return(false));
        EXPECT_CALL(*mock_reader, Initialized).WillOnce(Return(true));
    }

    TimePoint now{std::chrono::seconds(0)};
    data_systems::LazyLoad const lazy_load(logger, config, status_manager,
                                           [&]() { return now; });

    for (std::size_t i = 0; i < 10; i++) {
        ASSERT_FALSE(lazy_load.Initialized());
        now += std::chrono::seconds(1);
    }

    for (std::size_t i = 0; i < 10; i++) {
        ASSERT_TRUE(lazy_load.Initialized());
    }
}
