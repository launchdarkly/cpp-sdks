#include <gtest/gtest.h>

#include "all_flags_state/all_flags_state_builder.hpp"
#include "data_store/memory_store.hpp"
#include "test_store.hpp"

#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/logging/null_logger.hpp>
#include <launchdarkly/server_side/json_feature_flags_state.hpp>

#include <memory>

using namespace launchdarkly;
using namespace launchdarkly::server_side;

TEST(AllFlagsTest, Empty) {
    AllFlagsStateBuilder builder{AllFlagsStateOptions::Default};
    auto state = builder.Build();
    ASSERT_TRUE(state.Valid());
    ASSERT_TRUE(state.FlagsState().empty());
    ASSERT_TRUE(state.Evaluations().empty());
}

TEST(AllFlagsTest, DefaultOptions) {
    AllFlagsStateBuilder builder{AllFlagsStateOptions::Default};

    builder.AddFlag("myFlag", true,
                    AllFlagsState::Metadata{42, 1, std::nullopt, false, false,
                                            std::nullopt});

    auto state = builder.Build();
    ASSERT_TRUE(state.Valid());

    auto expected = boost::json::parse(R"({
        "myFlag": true,
        "$flagsState": {
            "myFlag": {
                "version": 42,
                "variation": 1
            }
        },
        "$valid": true
    })");

    auto got = boost::json::value_from(state);
    ASSERT_EQ(got, expected);
}

TEST(AllFlagsTest, DetailsOnlyForTrackedFlags) {
    AllFlagsStateBuilder builder{
        AllFlagsStateOptions::DetailsOnlyForTrackedFlags};
    builder.AddFlag(
        "myFlagTracked", true,
        AllFlagsState::Metadata{42, 1, EvaluationReason::Fallthrough(false),
                                true, true, std::nullopt});
    builder.AddFlag(
        "myFlagUntracked", true,
        AllFlagsState::Metadata{42, 1, EvaluationReason::Fallthrough(false),
                                false, false, std::nullopt});

    auto state = builder.Build();
    ASSERT_TRUE(state.Valid());

    auto expected = boost::json::parse(R"({
        "myFlagTracked" : true,
        "myFlagUntracked" : true,
        "$flagsState": {
            "myFlagTracked": {
                "version": 42,
                "variation": 1,
                "reason" : {
                    "kind": "FALLTHROUGH"
                },
                "trackReason" : true,
                "trackEvents" : true
            },
            "myFlagUntracked" : {
                "variation" : 1
            }
        },
        "$valid": true
    })");

    auto got = boost::json::value_from(state);
    ASSERT_EQ(got, expected);
}

TEST(AllFlagsTest, ClientSide) {
    AllFlagsStateBuilder builder{AllFlagsStateOptions::IncludeReasons};
    builder.AddFlag(
        "myFlag", true,
        AllFlagsState::Metadata{42, 1, EvaluationReason::Fallthrough(false),
                                false, false, std::nullopt});
    auto state = builder.Build();
    ASSERT_TRUE(state.Valid());

    auto expected = boost::json::parse(R"({
        "myFlag": true,
        "$flagsState": {
            "myFlag": {
                "version": 42,
                "variation": 1,
                "reason" : {
                    "kind": "FALLTHROUGH"
                }
            }
        },
        "$valid": true
    })");

    auto got = boost::json::value_from(state);
    ASSERT_EQ(got, expected);
}
