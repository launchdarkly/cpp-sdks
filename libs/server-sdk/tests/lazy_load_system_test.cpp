#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "data_systems/lazy_load/lazy_load_system.hpp"

#include <set>
#include <unordered_map>

using namespace launchdarkly::server_side;
using namespace launchdarkly::server_side::config;

using ::testing::NiceMock;

class MockDataReader : public data_interfaces::ISerializedDataReader {
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
};

class LazyLoadTest : public ::testing::Test {
   public:
    std::string mock_reader_name;
    std::shared_ptr<NiceMock<MockDataReader>> mock_reader;
    LazyLoadTest()
        : mock_reader_name("fake reader"),
          mock_reader(std::make_shared<NiceMock<MockDataReader>>()) {
        ON_CALL(*mock_reader, Identity()).WillByDefault([&]() {
            return mock_reader_name;
        });
    }
};

TEST_F(LazyLoadTest, ItentityWrapsReaderIdentity) {
    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled,
        std::chrono::milliseconds(100), mock_reader};

    data_systems::LazyLoad const lazy_load(config);

    ASSERT_EQ(lazy_load.Identity(), "lazy load via " + mock_reader_name);
}

TEST_F(LazyLoadTest, SourceIsNotAccessedIfFetchFails) {
    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled,
        std::chrono::seconds(10), mock_reader};

    data_systems::LazyLoad const lazy_load(config);

    EXPECT_CALL(*mock_reader, Get(testing::_, "foo"))
        .WillOnce(testing::Return(integrations::SerializedItemDescriptor{
            1, false, "{\"key\":\"foo\",\"version\":1}"}));

    // Although we ask for 'foo' 10 times, the underlying source should only
    // receive one call because the refresh is 10 seconds. Only after 10 seconds
    // elapse would the source be queried again.
    for (std::size_t i = 0; i < 10; i++) {
        ASSERT_FALSE(lazy_load.GetFlag("foo"));
    }
}

// TEST_F(LazyLoadTest, FetchingAllSegmentsRefreshesIndividualSegments) {
//     auto source = std::make_shared<FakeDataSource>();
//
//     auto refresh = std::chrono::seconds(10);
//
//     built::LazyLoadConfig const config{
//         built::LazyLoadConfig::EvictionPolicy::Disabled, refresh, source};
//
//     data_systems::LazyLoad const lazy_load(config);
//
//     ASSERT_FALSE(lazy_load.AllSegments().empty());
//     ASSERT_TRUE(lazy_load.GetSegment("foo"));
//
//     // Since all segments were requested, then..
//     ASSERT_EQ(source->all_requested, std::vector<std::string>{"segments"});
//     // There should be no individual request for a segment.
//     ASSERT_TRUE(source->items_requested["segments"].empty());
// }
//
// TEST_F(LazyLoadTest, FetchingAllFlagsRefreshesIndividualFlags) {
//     auto source = std::make_shared<FakeDataSource>();
//
//     auto refresh = std::chrono::seconds(10);
//
//     built::LazyLoadConfig const config{
//         built::LazyLoadConfig::EvictionPolicy::Disabled, refresh, source};
//
//     data_systems::LazyLoad const lazy_load(config);
//
//     ASSERT_FALSE(lazy_load.AllFlags().empty());
//     ASSERT_TRUE(lazy_load.GetFlag("foo"));
//
//     // Since all flags were requested, then..
//     ASSERT_EQ(source->all_requested, std::vector<std::string>{"features"});
//     // There should be no individual request for a segment.
//     ASSERT_TRUE(source->items_requested["features"].empty());
// }
//
// TEST_F(LazyLoadTest, ItemIsRefreshedAfterDelay) {
//     auto source = std::make_shared<FakeDataSource>();
//
//     auto refresh = std::chrono::seconds(10);
//
//     built::LazyLoadConfig const config{
//         built::LazyLoadConfig::EvictionPolicy::Disabled, refresh, source};
//
//     std::chrono::time_point<std::chrono::steady_clock> now(
//         std::chrono::seconds(0));
//
//     data_systems::LazyLoad const lazy_load(config, [&]() { return now; });
//
//     // Simulate time moving forward for 9 seconds. Each time, the flag should
//     be
//     // served from the cache rather than quering the store.
//     for (std::size_t i = 0; i < 9; i++) {
//         now = std::chrono::time_point<std::chrono::steady_clock>(
//             std::chrono::seconds(i));
//         ASSERT_TRUE(lazy_load.GetFlag("foo"));
//         ASSERT_EQ(source->items_requested["features"],
//                   std::vector<std::string>{"foo"});
//     }
//
//     // Advance past the refresh time. Now the flag should be queried again.
//     now = std::chrono::time_point<std::chrono::steady_clock>(refresh);
//
//     ASSERT_TRUE(lazy_load.GetFlag("foo"));
//     auto expected = std::vector<std::string>{"foo", "foo"};
//     ASSERT_EQ(source->items_requested["features"], expected);
// }
