#include <gtest/gtest.h>

#include <launchdarkly/data/evaluation_detail_internal.hpp>
#include <launchdarkly/data/evaluation_result.hpp>
#include <launchdarkly/client_side/data_source_update_sink.hpp>
#include <launchdarkly/client_side/flag_manager/detail/flag_manager.hpp>

using launchdarkly::EvaluationDetailInternal;
using launchdarkly::EvaluationResult;
using launchdarkly::Value;
using launchdarkly::client_side::ItemDescriptor;
using launchdarkly::client_side::flag_manager::detail::FlagManager;

TEST(FlagManagerTests, HandlesEmptyInit) {
    FlagManager manager;

    manager.Init(
        std::unordered_map<std::string,
                           launchdarkly::client_side::ItemDescriptor>{});

    EXPECT_TRUE(manager.GetAll().empty());
}

TEST(FlagManagerTests, HandlesInitWithData) {
    FlagManager manager;

    manager.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_EQ("test", manager.Get("flagA")->flag->detail().value());
}

TEST(FlagManagerTests, HandlesSecondInit) {
    FlagManager manager;

    manager.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    manager.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagB", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_EQ("test", manager.Get("flagB")->flag->detail().value());
    EXPECT_FALSE(manager.Get("flagA"));
}

TEST(FlagManagerTests, HandlePatchNewFlag) {
    FlagManager manager;

    manager.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    manager.Upsert("flagB",
                   ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_EQ("test", manager.Get("flagA")->flag->detail().value());
    EXPECT_EQ("second", manager.Get("flagB")->flag->detail().value());
}

TEST(FlagManagerTests, HandlePatchUpdateFlag) {
    FlagManager manager;

    manager.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    manager.Upsert("flagA",
                   ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_EQ("second", manager.Get("flagA")->flag->detail().value());
}

TEST(FlagManagerTests, HandleDelete) {
    FlagManager manager;

    manager.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    manager.Upsert("flagA", ItemDescriptor{2});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_FALSE(manager.Get("flagA")->flag.has_value());
}

TEST(FlagManagerTests, GetItemWhichDoesNotExist) {
    FlagManager manager;

    // Should be a null shared_ptr.
    EXPECT_FALSE(manager.Get("Potato"));
}
