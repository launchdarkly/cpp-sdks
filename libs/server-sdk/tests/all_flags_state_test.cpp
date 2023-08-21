#include <gtest/gtest.h>

#include "all_flags_state/all_flags_state_builder.hpp"

#include <launchdarkly/server_side/serialization/json_all_flags_state.hpp>

using namespace launchdarkly;
using namespace launchdarkly::server_side;

TEST(AllFlagsTest, Empty) {
    AllFlagsStateBuilder builder{AllFlagsState::Options::Default};
    auto state = builder.Build();
    ASSERT_TRUE(state.Valid());
    ASSERT_TRUE(state.States().empty());
    ASSERT_TRUE(state.Values().empty());
}

TEST(AllFlagsTest, DefaultOptions) {
    AllFlagsStateBuilder builder{AllFlagsState::Options::Default};

    builder.AddFlag(
        "myFlag", true,
        AllFlagsState::State{42, 1, std::nullopt, false, false, std::nullopt});

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
        AllFlagsState::Options::DetailsOnlyForTrackedFlags};
    builder.AddFlag(
        "myFlagTracked", true,
        AllFlagsState::State{42, 1, EvaluationReason::Fallthrough(false), true,
                             true, std::nullopt});
    builder.AddFlag(
        "myFlagUntracked", true,
        AllFlagsState::State{42, 1, EvaluationReason::Fallthrough(false), false,
                             false, std::nullopt});

    auto state = builder.Build();
    ASSERT_TRUE(state.Valid());

    auto expected = boost::json::parse(R"({
        "myFlagTracked" : true,
        "myFlagUntracked" : true,
        "$flagsState": {
            "myFlagTracked": {
                "version": 42,
                "variation": 1,
                "reason":{
                    "kind" : "FALLTHROUGH"
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

TEST(AllFlagsTest, IncludeReasons) {
    AllFlagsStateBuilder builder{AllFlagsState::Options::IncludeReasons};
    builder.AddFlag(
        "myFlag", true,
        AllFlagsState::State{42, 1, EvaluationReason::Fallthrough(false), false,
                             false, std::nullopt});
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

TEST(AllFlagsTest, FlagValues) {
    AllFlagsStateBuilder builder{AllFlagsState::Options::Default};

    const std::size_t kNumFlags = 10;

    for (std::size_t i = 0; i < kNumFlags; i++) {
        builder.AddFlag("myFlag" + std::to_string(i), "value",
                        AllFlagsState::State{42, 1, std::nullopt, false, false,
                                             std::nullopt});
    }

    auto state = builder.Build();

    auto const& vals = state.Values();

    ASSERT_EQ(vals.size(), kNumFlags);

    ASSERT_TRUE(std::all_of(vals.begin(), vals.end(), [](auto const& kvp) {
        return kvp.second.AsString() == "value";
    }));
}

TEST(AllFlagsTest, FlagState) {
    AllFlagsStateBuilder builder{AllFlagsState::Options::Default};

    const std::size_t kNumFlags = 10;

    AllFlagsState::State state{42, 1, std::nullopt, false, false, std::nullopt};
    for (std::size_t i = 0; i < kNumFlags; i++) {
        builder.AddFlag("myFlag" + std::to_string(i), "value", state);
    }

    auto all_flags_state = builder.Build();

    auto const& states = all_flags_state.States();

    ASSERT_EQ(states.size(), kNumFlags);

    ASSERT_TRUE(std::all_of(states.begin(), states.end(), [&](auto const& kvp) {
        return kvp.second == state;
    }));
}
