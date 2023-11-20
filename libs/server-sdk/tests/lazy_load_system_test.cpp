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
    FakeDataSource(std::string name)
        : name(std::move(name)), items_requested() {}
    GetResult Get(integrations::IPersistentKind const& kind,
                  std::string const& itemKey) const override {
        items_requested[kind.Namespace()].push_back(itemKey);
        return tl::make_unexpected(
            ISerializedDataPullSource::Error{"no such flag"});
    }
    AllResult All(integrations::IPersistentKind const& kind) const override {}

    std::string const& Identity() const override { return name; }
    bool Initialized() const override { return true; }

    std::string name;
    mutable std::unordered_map<std::string, std::vector<std::string>>
        items_requested;
};

TEST_F(LazyLoadTest, ItentityIncludesSourceIdentity) {
    std::string const name = "fake source";

    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled,
        std::chrono::milliseconds(100), std::make_shared<FakeDataSource>(name)};

    data_systems::LazyLoad const lazy_load(config);

    ASSERT_EQ(lazy_load.Identity(), "lazy load via " + name);
}

TEST_F(LazyLoadTest, SourceIsAccessedRepeatedlyIfFetchingFails) {
    auto source = std::make_shared<FakeDataSource>("fake source");

    // We want the cache refresh logic to not play a role in this unit test, so
    // set the refresh large enough that it's highly unlikely to be triggered.
    auto refresh = std::chrono::seconds(10);

    built::LazyLoadConfig const config{
        built::LazyLoadConfig::EvictionPolicy::Disabled, refresh, source};

    data_systems::LazyLoad const lazy_load(config);

    constexpr std::size_t kNumCalls = 10;

    // Each call will need to hit the source because the FakeDataSource always
    // responds with an error.
    for (std::size_t i = 0; i < kNumCalls; i++) {
        ASSERT_FALSE(lazy_load.GetFlag("foo"));
    }

    ASSERT_EQ(source->items_requested["features"].size(), kNumCalls);
}
