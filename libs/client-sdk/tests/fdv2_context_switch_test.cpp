#include <gtest/gtest.h>

#include <data_sources/fdv2/cache_initializer.hpp>
#include <flag_manager/flag_manager.hpp>

#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/logging/null_logger.hpp>

#include <map>
#include <memory>
#include <optional>
#include <string>

using launchdarkly::Context;
using launchdarkly::ContextBuilder;
using launchdarkly::EvaluationDetailInternal;
using launchdarkly::EvaluationResult;
using launchdarkly::Value;
using launchdarkly::client_side::FlagChange;
using launchdarkly::client_side::FlagChangeSet;
using launchdarkly::client_side::ItemDescriptor;
using launchdarkly::client_side::flag_manager::FlagManager;
using launchdarkly::client_side::flag_manager::PersistenceEncodeKey;
using launchdarkly::data_model::ChangeSetType;
using launchdarkly::data_model::Selector;

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

ItemDescriptor Flag(std::uint64_t version, Value value) {
    return ItemDescriptor{
        EvaluationResult{version, std::nullopt, false, false, std::nullopt,
                         EvaluationDetailInternal{std::move(value),
                                                  std::nullopt, std::nullopt}}};
}

char const* const kEnvironment =
    "LaunchDarkly_rUTcjlHPv6Vegd27YmtGYkEGkEUGaEbn5M0JYTFQUpA=";

}  // namespace

// A selector names a state the service can compute changes against for one
// context. Sending it for another would ask for the wrong delta.
TEST(FDv2ContextSwitchTest, ClearSelectorForgetsTheBasisButKeepsTheData) {
    auto logger = launchdarkly::logging::NullLogger();
    FlagManager flag_manager("the-key", logger, 5, nullptr);

    flag_manager.Updater().Apply(
        ContextBuilder().Kind("user", "first").Build(),
        FlagChangeSet{ChangeSetType::kFull,
                      {FlagChange{"flagA", Flag(1, Value("a"))}},
                      Selector{Selector::State{1, "state-1"}}},
        /* from_cache= */ false);

    ASSERT_TRUE(flag_manager.Store().CurrentSelector().value.has_value());

    flag_manager.ClearSelector();

    EXPECT_FALSE(flag_manager.Store().CurrentSelector().value.has_value());
    ASSERT_TRUE(flag_manager.Store().Get("flagA"));
    EXPECT_EQ(Value("a"),
              flag_manager.Store().Get("flagA")->item->Detail().Value());
}

// Nothing is available to evaluate against for the new context yet, so the
// previous context's data has to stay until a full data set arrives.
TEST(FDv2ContextSwitchTest, CacheMissRetainsThePreviousContextsData) {
    auto logger = launchdarkly::logging::NullLogger();
    auto persistence =
        std::make_shared<TestPersistence>(TestPersistence::StoreType());
    FlagManager flag_manager("the-key", logger, 5, persistence);

    auto first = ContextBuilder().Kind("user", "first").Build();
    flag_manager.Updater().Apply(
        first,
        FlagChangeSet{ChangeSetType::kFull,
                      {FlagChange{"flagA", Flag(1, Value("first-value"))}},
                      Selector{Selector::State{1, "state-1"}}},
        /* from_cache= */ false);

    auto second = ContextBuilder().Kind("user", "second").Build();
    flag_manager.ClearSelector();
    flag_manager.LoadCache(second);

    ASSERT_TRUE(flag_manager.Store().Get("flagA"));
    EXPECT_EQ(Value("first-value"),
              flag_manager.Store().Get("flagA")->item->Detail().Value());
}

TEST(FDv2ContextSwitchTest, CacheHitReplacesThePreviousContextsData) {
    auto logger = launchdarkly::logging::NullLogger();
    auto second = ContextBuilder().Kind("user", "second").Build();
    auto persistence =
        std::make_shared<TestPersistence>(TestPersistence::StoreType{
            {kEnvironment,
             {{PersistenceEncodeKey(second.CanonicalKey()),
               R"({"flagB":{"version":1,"value":"second-value"}})"}}}});
    FlagManager flag_manager("the-key", logger, 5, persistence);

    flag_manager.Updater().Apply(
        ContextBuilder().Kind("user", "first").Build(),
        FlagChangeSet{ChangeSetType::kFull,
                      {FlagChange{"flagA", Flag(1, Value("first-value"))}},
                      Selector{Selector::State{1, "state-1"}}},
        /* from_cache= */ false);

    flag_manager.ClearSelector();
    flag_manager.LoadCache(second);

    EXPECT_FALSE(flag_manager.Store().Get("flagA"));
    ASSERT_TRUE(flag_manager.Store().Get("flagB"));
    EXPECT_EQ(Value("second-value"),
              flag_manager.Store().Get("flagB")->item->Detail().Value());
}

// The cache initializer for a new context reports a hit or a miss for that
// context alone, whatever the store currently holds.
TEST(FDv2ContextSwitchTest, CacheInitializerReadsTheNewContext) {
    auto logger = launchdarkly::logging::NullLogger();
    auto first = ContextBuilder().Kind("user", "first").Build();
    auto second = ContextBuilder().Kind("user", "second").Build();
    auto persistence =
        std::make_shared<TestPersistence>(TestPersistence::StoreType{
            {kEnvironment,
             {{PersistenceEncodeKey(first.CanonicalKey()),
               R"({"flagA":{"version":1,"value":"first-value"}})"}}}});
    FlagManager flag_manager("the-key", logger, 5, persistence);

    using launchdarkly::client_side::data_sources::FDv2CacheInitializer;
    using launchdarkly::client_side::data_sources::FDv2SourceResult;

    auto second_result =
        FDv2CacheInitializer(&flag_manager.Cache(), second, logger)
            .Run()
            .GetResult();
    auto* second_change_set =
        std::get_if<FDv2SourceResult::ChangeSet>(&second_result->value);
    ASSERT_NE(nullptr, second_change_set);
    EXPECT_EQ(ChangeSetType::kNone, second_change_set->change_set.type);

    auto first_result =
        FDv2CacheInitializer(&flag_manager.Cache(), first, logger)
            .Run()
            .GetResult();
    auto* first_change_set =
        std::get_if<FDv2SourceResult::ChangeSet>(&first_result->value);
    ASSERT_NE(nullptr, first_change_set);
    EXPECT_EQ(ChangeSetType::kFull, first_change_set->change_set.type);
}
