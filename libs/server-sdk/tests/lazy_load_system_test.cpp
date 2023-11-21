#include <gtest/gtest.h>

#include "data_systems/lazy_load/lazy_load_system.hpp"

#include <set>
#include <unordered_map>

using namespace launchdarkly::server_side;
using namespace launchdarkly::server_side::config;

class LazyLoadTest : public ::testing::Test {};

class FakeDataSource : public launchdarkly::server_side::data_interfaces::
                           ISerializedDataPullSource {
   public:
    using GetFn = std::function<GetResult(integrations::ISerializedItemKind const&,
                                          std::string const& key)>;

    using AllFn =
        std::function<AllResult(integrations::ISerializedItemKind const&)>;

    FakeDataSource(std::string name, GetFn items, AllFn all)
        : name(std::move(name)),
          item_getter([](integrations::ISerializedItemKind const& kind,
                         std::string const& key) {
              return tl::make_unexpected(Error{key + "not found"});
          }),
          all_getter([](integrations::ISerializedItemKind const& kind) {
              return tl::make_unexpected(Error{kind.Namespace() + "not found"});
          }),
          items_requested() {
        if (items) {
            item_getter = std::move(items);
        }
        if (all) {
            all_getter = std::move(all);
        }
    }

    explicit FakeDataSource(std::string name)
        : FakeDataSource(std::move(name), nullptr, nullptr) {}

    FakeDataSource() : FakeDataSource("fake source") {}

    GetResult Get(integrations::ISerializedItemKind const& kind,
                  std::string const& itemKey) const override {
        items_requested[kind.Namespace()].push_back(itemKey);
        return item_getter(kind, itemKey);
    }
    AllResult All(integrations::ISerializedItemKind const& kind) const override {
        all_requested.push_back(kind.Namespace());
        return all_getter(kind);
    }

    std::string const& Identity() const override { return name; }
    bool Initialized() const override { return true; }

    std::string name;
    GetFn item_getter;
    AllFn all_getter;
    mutable std::unordered_map<std::string, std::vector<std::string>>
        items_requested;
    mutable std::vector<std::string> all_requested;
};

TEST_F(LazyLoadTest, ItentityIncludesSourceIdentity) {
    std::string const name = "fake source";

    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled,
        std::chrono::milliseconds(100), std::make_shared<FakeDataSource>(name)};

    data_systems::LazyLoad const lazy_load(config);

    ASSERT_EQ(lazy_load.Identity(), "lazy load via " + name);
}

TEST_F(LazyLoadTest, SourceIsNotAccessedIfFetchFails) {
    auto source = std::make_shared<FakeDataSource>();

    auto refresh = std::chrono::seconds(10);

    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled, refresh, source};

    data_systems::LazyLoad const lazy_load(config);

    // Although we ask for 'foo' 10 times, the underlying source should only
    // receive one call because the refresh is 10 seconds. Only after 10 seconds
    // elapse would the source be queried again.
    for (std::size_t i = 0; i < 10; i++) {
        ASSERT_FALSE(lazy_load.GetFlag("foo"));
    }

    ASSERT_EQ(source->items_requested["features"].size(), 1);
}

TEST_F(LazyLoadTest, FetchingAllSegmentsRefreshesIndividualSegments) {
    auto source = std::make_shared<FakeDataSource>();

    auto refresh = std::chrono::seconds(10);

    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled, refresh, source};

    data_systems::LazyLoad const lazy_load(config);

    ASSERT_FALSE(lazy_load.AllSegments().empty());
    ASSERT_TRUE(lazy_load.GetSegment("foo"));

    // Since all segments were requested, then..
    ASSERT_EQ(source->all_requested, std::vector<std::string>{"segments"});
    // There should be no individual request for a segment.
    ASSERT_TRUE(source->items_requested["segments"].empty());
}

TEST_F(LazyLoadTest, FetchingAllFlagsRefreshesIndividualFlags) {
    auto source = std::make_shared<FakeDataSource>();

    auto refresh = std::chrono::seconds(10);

    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled, refresh, source};

    data_systems::LazyLoad const lazy_load(config);

    ASSERT_FALSE(lazy_load.AllFlags().empty());
    ASSERT_TRUE(lazy_load.GetFlag("foo"));

    // Since all flags were requested, then..
    ASSERT_EQ(source->all_requested, std::vector<std::string>{"features"});
    // There should be no individual request for a segment.
    ASSERT_TRUE(source->items_requested["features"].empty());
}

TEST_F(LazyLoadTest, ItemIsRefreshedAfterDelay) {
    auto source = std::make_shared<FakeDataSource>();

    auto refresh = std::chrono::seconds(10);

    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled, refresh, source};

    std::chrono::time_point<std::chrono::steady_clock> now(
        std::chrono::seconds(0));

    data_systems::LazyLoad const lazy_load(config, [&]() { return now; });

    // Simulate time moving forward for 9 seconds. Each time, the flag should be
    // served from the cache rather than quering the store.
    for (std::size_t i = 0; i < 9; i++) {
        now = std::chrono::time_point<std::chrono::steady_clock>(
            std::chrono::seconds(i));
        ASSERT_TRUE(lazy_load.GetFlag("foo"));
        ASSERT_EQ(source->items_requested["features"],
                  std::vector<std::string>{"foo"});
    }

    // Advance past the refresh time. Now the flag should be queried again.
    now = std::chrono::time_point<std::chrono::steady_clock>(refresh);

    ASSERT_TRUE(lazy_load.GetFlag("foo"));
    auto expected = std::vector<std::string>{"foo", "foo"};
    ASSERT_EQ(source->items_requested["features"], expected);
}
