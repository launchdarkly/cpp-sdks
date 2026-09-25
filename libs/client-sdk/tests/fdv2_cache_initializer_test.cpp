#include <gtest/gtest.h>

#include <data_sources/fdv2/cache_initializer.hpp>
#include <flag_manager/flag_manager.hpp>

#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/logging/null_logger.hpp>

#include <map>
#include <memory>
#include <optional>
#include <string>

using launchdarkly::ContextBuilder;
using launchdarkly::Value;
using launchdarkly::client_side::flag_manager::FlagManager;
using launchdarkly::client_side::flag_manager::PersistenceEncodeKey;
using launchdarkly::data_model::ChangeSetType;

using namespace launchdarkly::client_side::data_sources;

namespace {

class TestPersistence : public IPersistence {
   public:
    using StoreType =
        std::map<std::string,
                 std::map<std::string, std::optional<std::string>>>;

    explicit TestPersistence(StoreType store) : store_(std::move(store)) {}

    void Set(std::string storageNamespace,
             std::string key,
             std::string data) noexcept override {
        store_[storageNamespace][key] = data;
    }

    void Remove(std::string storageNamespace,
                std::string key) noexcept override {
        store_[storageNamespace].erase(key);
    }

    std::optional<std::string> Read(std::string storageNamespace,
                                    std::string key) noexcept override {
        return store_[storageNamespace][key];
    }

    StoreType store_;
};

// The environment namespace and context id the client derives for SDK key
// "the-key" and context user:user-key.
char const* const kEnvironment =
    "LaunchDarkly_rUTcjlHPv6Vegd27YmtGYkEGkEUGaEbn5M0JYTFQUpA=";
char const* const kContextId = "CEXjZY7cHJG_ydFy7q4-YEFwVrG3_pkJwA4FAjrbfx0=";

}  // namespace

TEST(FDv2CacheInitializerTest, CacheHitProducesAFullDataSet) {
    auto context = ContextBuilder().Kind("user", "user-key").Build();
    auto logger = launchdarkly::logging::NullLogger();
    auto persistence =
        std::make_shared<TestPersistence>(TestPersistence::StoreType{
            {kEnvironment,
             {{kContextId, R"({"flagA":{"version":1,"value":"test"}})"}}}});
    FlagManager flag_manager("the-key", logger, 5, persistence);

    FDv2CacheInitializer initializer(&flag_manager.Cache(), context, logger);
    auto future = initializer.Run();
    ASSERT_TRUE(future.IsFinished());
    auto result = future.GetResult();

    // The result is a full change set carrying the cached flag.
    auto* change_set = std::get_if<FDv2SourceResult::ChangeSet>(&result->value);
    ASSERT_NE(nullptr, change_set);
    EXPECT_EQ(ChangeSetType::kFull, change_set->change_set.type);
    ASSERT_EQ(1u, change_set->change_set.data.size());
    EXPECT_EQ("flagA", change_set->change_set.data[0].key);
    EXPECT_EQ(Value("test"),
              change_set->change_set.data[0].item.item->Detail().Value());

    // A delta against unverified cached data could silently corrupt the store,
    // so the cache supplies no selector.
    EXPECT_FALSE(change_set->change_set.selector.value.has_value());
}

// A miss leaves the data set unchanged and lets the chain proceed, rather than
// reporting an error.
TEST(FDv2CacheInitializerTest, CacheMissProducesANoneIntent) {
    auto context = ContextBuilder().Kind("user", "unknown").Build();
    auto logger = launchdarkly::logging::NullLogger();
    auto persistence =
        std::make_shared<TestPersistence>(TestPersistence::StoreType());
    FlagManager flag_manager("the-key", logger, 5, persistence);

    FDv2CacheInitializer initializer(&flag_manager.Cache(), context, logger);
    auto future = initializer.Run();
    ASSERT_TRUE(future.IsFinished());
    auto result = future.GetResult();

    auto* change_set = std::get_if<FDv2SourceResult::ChangeSet>(&result->value);
    ASSERT_NE(nullptr, change_set);
    EXPECT_EQ(ChangeSetType::kNone, change_set->change_set.type);
    EXPECT_TRUE(change_set->change_set.data.empty());
}

TEST(FDv2CacheInitializerTest, NoPersistenceConfiguredProducesANoneIntent) {
    auto context = ContextBuilder().Kind("user", "user-key").Build();
    auto logger = launchdarkly::logging::NullLogger();
    FlagManager flag_manager("the-key", logger, 5, nullptr);

    FDv2CacheInitializer initializer(&flag_manager.Cache(), context, logger);
    auto future = initializer.Run();
    ASSERT_TRUE(future.IsFinished());
    auto result = future.GetResult();

    // Not configuring persistence acts like a cache miss, not an error.
    auto* change_set = std::get_if<FDv2SourceResult::ChangeSet>(&result->value);
    ASSERT_NE(nullptr, change_set);
    EXPECT_EQ(ChangeSetType::kNone, change_set->change_set.type);
}

// The orchestrator needs to tell cache initializers apart from network ones,
// so that a miss with nothing else configured still starts the SDK.
TEST(FDv2CacheInitializerTest, FactoryIdentifiesItselfAsReadingTheCache) {
    auto context = ContextBuilder().Kind("user", "user-key").Build();
    auto logger = launchdarkly::logging::NullLogger();
    FlagManager flag_manager("the-key", logger, 5, nullptr);

    FDv2CacheInitializerFactory factory(&flag_manager.Cache(), context, logger);

    EXPECT_TRUE(factory.IsFromCache());
    EXPECT_NE(nullptr, factory.Build());
}
