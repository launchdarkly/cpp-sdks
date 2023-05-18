#include <gtest/gtest.h>

#include "data_sources/data_source_update_sink.hpp"
#include "flag_manager/flag_persistence.hpp"
#include "flag_manager/flag_updater.hpp"

#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/logging/null_logger.hpp>

using launchdarkly::ContextBuilder;
using launchdarkly::EvaluationDetailInternal;
using launchdarkly::EvaluationResult;
using launchdarkly::Value;
using launchdarkly::client_side::ItemDescriptor;
using launchdarkly::client_side::flag_manager::FlagPersistence;
using launchdarkly::client_side::flag_manager::FlagStore;
using launchdarkly::client_side::flag_manager::FlagUpdater;

class TestPersistence : public IPersistence {
   public:
    using StoreType =
        std::map<std::string,
                 std::map<std::string, std::optional<std::string>>>;
    TestPersistence(StoreType store) : store_(std::move(store)) {}

    void SetValue(std::string storageNamespace,
                  std::string key,
                  std::string data) noexcept override {
        store_[storageNamespace][key] = data;
    };

    void RemoveValue(std::string storageNamespace,
                     std::string key) noexcept override {
        auto& space = store_[storageNamespace];
        auto found = space.find(key);
        if (found != space.end()) {
            space.erase(found);
        }
    };

    std::optional<std::string> Read(std::string storageNamespace,
                                    std::string key) noexcept override {
        return store_[storageNamespace][key];
    };

    StoreType store_;
};

TEST(FlagPersistenceTests, StoresCacheOnInit) {
    auto context = ContextBuilder().kind("user", "user-key").build();
    auto store = FlagStore();
    auto updater = FlagUpdater(store);
    auto persistence =
        std::make_shared<TestPersistence>(TestPersistence::StoreType());
    auto logger = launchdarkly::logging::NullLogger();

    FlagPersistence flag_persistence(
        "the-key", &updater, store, persistence, logger, 5, []() {
            return std::chrono::system_clock::time_point{
                std::chrono::milliseconds{500}};
        });
    flag_persistence.Init(
        context,
        std::unordered_map<std::string,
                           launchdarkly::client_side::ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    // Only 1 namespace.
    EXPECT_EQ(1, persistence->store_.size());
    // Index + 1 context.
    EXPECT_EQ(2, persistence->store_.begin()->second.size());

    // Created the index.
    EXPECT_EQ(
        R"({"index":[{"id":"0845e3658edc1c91bfc9d172eeae3e60417056b1b7fe9909c00e05023adb7f1d","timestamp":500}]})",
        persistence->store_
            ["LaunchDarkly_"
             "ad44dc8e51cfbfa55e81ddbb626b466241069045066846e7e4cd096131505290"]
            ["ContextIndex"]);

    // Put the cache in for the specific context.
    EXPECT_EQ(
        R"({"flagA":{"version":1,"value":"test"}})",
        persistence->store_
            ["LaunchDarkly_"
             "ad44dc8e51cfbfa55e81ddbb626b466241069045066846e7e4cd096131505290"]
            ["0845e3658edc1c91bfc9d172eeae3e60417056b1b7fe9909c00e05023adb7f1"
             "d"]);
}

TEST(FlagPersistenceTests, CanLoadCache) {
    auto context = ContextBuilder().kind("user", "user-key").build();
    auto store = FlagStore();
    auto updater = FlagUpdater(store);
    auto logger = launchdarkly::logging::NullLogger();

    auto persistence = std::make_shared<
        TestPersistence>(TestPersistence::StoreType{
        {"LaunchDarkly_"
         "ad44dc8e51cfbfa55e81ddbb626b466241069045066846e7e4cd096131505290",
         {{"ContextIndex",
           R"([{"id":"0845e3658edc1c91bfc9d172eeae3e60417056b1b7fe9909c00e05023adb7f1d","timestamp":500}])"},
          {"0845e3658edc1c91bfc9d172eeae3e60417056b1b7fe9909c00e05023adb7f1d",
           R"({"flagA":{"version":1,"value":"test"}})"}}}});

    FlagPersistence flag_persistence("the-key", &updater, store, persistence,
                                     logger, 5);

    flag_persistence.LoadCached(context);

    // The store contains the flag loaded from the persistence.
    EXPECT_EQ("test", store.Get("flagA")->flag->detail().value().AsString());
}

TEST(FlagPersistenceTests, EvictsContextsBeyondMax) {
    auto store = FlagStore();
    auto updater = FlagUpdater(store);
    auto logger = launchdarkly::logging::NullLogger();

    auto persistence =
        std::make_shared<TestPersistence>(TestPersistence::StoreType());

    uint64_t now = 0;

    // Set the max contexts to 2.
    FlagPersistence flag_persistence(
        "the-key", &updater, store, persistence, logger, 2, [&now]() {
            return std::chrono::system_clock::time_point{
                std::chrono::milliseconds{now}};
        });

    flag_persistence.Init(
        ContextBuilder().kind("potato", "user-key").build(),
        std::unordered_map<std::string,
                           launchdarkly::client_side::ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    now++;

    flag_persistence.Init(
        ContextBuilder().kind("potato", "bob-key").build(),
        std::unordered_map<std::string,
                           launchdarkly::client_side::ItemDescriptor>{
            {{"flagB", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    now++;

    flag_persistence.Init(
        ContextBuilder().kind("potato", "susan-key").build(),
        std::unordered_map<std::string,
                           launchdarkly::client_side::ItemDescriptor>{
            {{"flagC", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    // Only 1 namespace.
    EXPECT_EQ(1, persistence->store_.size());

    // Index + 2 Contexts
    auto& space = persistence->store_.begin()->second;
    EXPECT_EQ(3, space.size());
    // Sha256 potato:user-key
    EXPECT_EQ(0, space.count("423dbc8f0d4b15f7b2c466479be01beb4ded297f9d4efdce9"
                             "08ecb6182341495"));
    // Sha256 potato:bob-key
    EXPECT_EQ(1, space.count("4d88492463db48c874deef4b87a96c952dc6d192a0ed2f0cf"
                             "ee88485548e4035"));
    // Sha256 potato:susan-key
    EXPECT_EQ(1, space.count("2805a9631ca563307b71b1533acb1f74f938418ebdfe91f5a"
                             "728e792f515831a"));
}
