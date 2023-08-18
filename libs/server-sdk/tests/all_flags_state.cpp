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

class AllFlagsTest : public ::testing::Test {
   protected:
    AllFlagsTest()
        : logger_(logging::NullLogger()),
          store_(test_store::Empty()),
          evaluator_(logger_, *store_),
          builder_(evaluator_, *store_),
          context_(ContextBuilder().Kind("shadow", "cat").Build()) {}
    Logger logger_;
    std::unique_ptr<data_store::IDataStore> store_;
    evaluation::Evaluator evaluator_;
    AllFlagsStateBuilder builder_;
    Context context_;
};

TEST_F(AllFlagsTest, Empty) {
    auto state = builder_.Build(context_, AllFlagsStateOptions::Default);
    ASSERT_TRUE(state.Valid());
    ASSERT_TRUE(state.FlagsState().empty());
    ASSERT_TRUE(state.Evaluations().empty());
}

TEST_F(AllFlagsTest, DefaultOptions) {
    auto mem_store = static_cast<data_store::MemoryStore*>(store_.get());
    mem_store->Upsert("myFlag", test_store::Flag(R"({
        "key": "myFlag",
        "version": 42,
        "on": true,
        "targets": [],
        "rules": [],
        "prerequisites": [],
        "fallthrough": {"variation": 1},
        "offVariation": 0,
        "variations": [false, true],
        "clientSideAvailability": {
            "usingMobileKey": false,
            "usingEnvironmentId": false
        },
        "salt": "kosher"
    })"));

    auto state = builder_.Build(context_, AllFlagsStateOptions::Default);
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

TEST_F(AllFlagsTest, HandlesExperimentationReasons) {}
