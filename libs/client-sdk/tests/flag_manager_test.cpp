#include <gtest/gtest.h>

#include "data/evaluation_detail_internal.hpp"
#include "data/evaluation_result.hpp"
#include "launchdarkly/client_side/data_source_update_sink.hpp"
#include "launchdarkly/client_side/flag_manager/detail/flag_manager.hpp"

using launchdarkly::EvaluationDetailInternal;
using launchdarkly::EvaluationResult;
using launchdarkly::Value;
using launchdarkly::client_side::ItemDescriptor;
using launchdarkly::client_side::flag_manager::detail::FlagManager;

TEST(FlagManagerTests, HandlesEmptyInit) {
    FlagManager manager;

    manager.init(
        std::unordered_map<std::string,
                           launchdarkly::client_side::ItemDescriptor>{});

    EXPECT_TRUE(manager.get_all().empty());
}

TEST(FlagManagerTests, HandlesInitWithData) {
    FlagManager manager;

    manager.init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    EXPECT_FALSE(manager.get_all().empty());
    EXPECT_EQ("test", manager.get("flagA")->flag.value().detail().value());
}

TEST(FlagManagerTests, HandlesSecondInit) {
    FlagManager manager;

    manager.init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    manager.init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagB", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    EXPECT_FALSE(manager.get_all().empty());
    EXPECT_EQ("test", manager.get("flagB")->flag.value().detail().value());
    EXPECT_FALSE(manager.get("flagA"));
}

TEST(FlagManagerTests, HandlePatchNewFlag) {
    FlagManager manager;

    manager.init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    manager.upsert("flagB",
                   ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_FALSE(manager.get_all().empty());
    EXPECT_EQ("test", manager.get("flagA")->flag.value().detail().value());
    EXPECT_EQ("second", manager.get("flagB")->flag.value().detail().value());
}

TEST(FlagManagerTests, HandlePatchUpdateFlag) {
    FlagManager manager;

    manager.init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    manager.upsert("flagA",
                   ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_FALSE(manager.get_all().empty());
    EXPECT_EQ("second", manager.get("flagA")->flag.value().detail().value());
}

TEST(FlagManagerTests, HandleDelete) {
    FlagManager manager;

    manager.init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    manager.upsert("flagA", ItemDescriptor{2});

    EXPECT_FALSE(manager.get_all().empty());
    EXPECT_FALSE(manager.get("flagA")->flag.has_value());
}

TEST(FlagManagerTests, GetItemWhichDoesNotExist) {
    FlagManager manager;

    // Should be a null shared_ptr.
    EXPECT_FALSE(manager.get("Potato"));
}
