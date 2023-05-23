#include <gtest/gtest.h>

#include <launchdarkly/data/evaluation_detail_internal.hpp>
#include <launchdarkly/data/evaluation_result.hpp>

#include "data_sources/data_source_update_sink.hpp"
#include "flag_manager/flag_store.hpp"

using launchdarkly::EvaluationDetailInternal;
using launchdarkly::EvaluationResult;
using launchdarkly::Value;
using launchdarkly::client_side::ItemDescriptor;
using launchdarkly::client_side::flag_manager::FlagStore;

TEST(FlagstoreTests, HandlesEmptyInit) {
    FlagStore store;

    store.Init(std::unordered_map<std::string,
                                  launchdarkly::client_side::ItemDescriptor>{});

    EXPECT_TRUE(store.GetAll().empty());
}

TEST(FlagstoreTests, HandlesInitWithData) {
    FlagStore store;

    store.Init(std::unordered_map<std::string,
                                  launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    EXPECT_FALSE(store.GetAll().empty());
    EXPECT_EQ("test", store.Get("flagA")->flag->Detail().Value());
}

TEST(FlagstoreTests, HandlesSecondInit) {
    FlagStore store;

    store.Init(std::unordered_map<std::string,
                                  launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    store.Init(std::unordered_map<std::string,
                                  launchdarkly::client_side::ItemDescriptor>{
        {{"flagB", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    EXPECT_FALSE(store.GetAll().empty());
    EXPECT_EQ("test", store.Get("flagB")->flag->Detail().Value());
    EXPECT_FALSE(store.Get("flagA"));
}

TEST(FlagstoreTests, HandlePatchNewFlag) {
    FlagStore store;

    store.Init(std::unordered_map<std::string,
                                  launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    store.Upsert("flagB",
                 ItemDescriptor{EvaluationResult{
                     0, std::nullopt, false, false, std::nullopt,
                     EvaluationDetailInternal{Value("second"), std::nullopt,
                                              std::nullopt}}});

    EXPECT_FALSE(store.GetAll().empty());
    EXPECT_EQ("test", store.Get("flagA")->flag->Detail().Value());
    EXPECT_EQ("second", store.Get("flagB")->flag->Detail().Value());
}

TEST(FlagstoreTests, HandlePatchUpdateFlag) {
    FlagStore store;

    store.Init(std::unordered_map<std::string,
                                  launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    store.Upsert("flagA",
                 ItemDescriptor{EvaluationResult{
                     1, std::nullopt, false, false, std::nullopt,
                     EvaluationDetailInternal{Value("second"), std::nullopt,
                                              std::nullopt}}});

    EXPECT_FALSE(store.GetAll().empty());
    EXPECT_EQ("second", store.Get("flagA")->flag->Detail().Value());
}

TEST(FlagstoreTests, HandleDelete) {
    FlagStore store;

    store.Init(std::unordered_map<std::string,
                                  launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    store.Upsert("flagA", ItemDescriptor{2});

    EXPECT_FALSE(store.GetAll().empty());
    EXPECT_FALSE(store.Get("flagA")->flag.has_value());
}

TEST(FlagstoreTests, GetItemWhichDoesNotExist) {
    FlagStore store;

    // Should be a null shared_ptr.
    EXPECT_FALSE(store.Get("Potato"));
}
