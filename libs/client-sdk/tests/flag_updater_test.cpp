#include <gtest/gtest.h>

#include "data/evaluation_detail_internal.hpp"
#include "data/evaluation_result.hpp"
#include "launchdarkly/client_side/data_source_update_sink.hpp"
#include "launchdarkly/client_side/flag_manager/detail/flag_manager.hpp"
#include "launchdarkly/client_side/flag_manager/detail/flag_updater.hpp"
#include "value.hpp"

using launchdarkly::EvaluationDetailInternal;
using launchdarkly::EvaluationResult;
using launchdarkly::Value;
using launchdarkly::client_side::ItemDescriptor;
using launchdarkly::client_side::flag_manager::detail::FlagManager;
using launchdarkly::client_side::flag_manager::detail::FlagUpdater;
using launchdarkly::client_side::flag_manager::detail::FlagValueChangeEvent;

TEST(FlagUpdaterTests, HandlesEmptyInit) {
    FlagManager manager;
    FlagUpdater updater(manager);

    updater.init(
        std::unordered_map<std::string,
                           launchdarkly::client_side::ItemDescriptor>{});

    EXPECT_TRUE(manager.get_all().empty());
}

TEST(FlagUpdaterTests, HandledInitWithData) {
    FlagManager manager;
    FlagUpdater updater(manager);

    updater.init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    EXPECT_FALSE(manager.get_all().empty());
    EXPECT_EQ("test", manager.get("flagA")->flag.value().detail().value());
}