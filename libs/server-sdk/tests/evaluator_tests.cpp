#include <gtest/gtest.h>
#include <launchdarkly/logging/null_logger.hpp>
#include <launchdarkly/logging/spy_logger.hpp>

#include "evaluation/evaluator.hpp"
#include "test_store.hpp"

#include <launchdarkly/context.hpp>
#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/data_model/flag.hpp>

using namespace launchdarkly;
using namespace launchdarkly::server_side;

class EvaluatorTests : public ::testing::Test {
   public:
    EvaluatorTests()
        : store(test_store::TestData()), logger(logging::NullLogger()) {}
    std::unique_ptr<data_store::IDataStore> store;
    Logger logger;
};

TEST_F(EvaluatorTests, BasicChanges) {
    evaluation::Evaluator e(logger, *store);

    auto alice = ContextBuilder().Kind("user", "alice").Build();
    auto bob = ContextBuilder().Kind("user", "bob").Build();

    auto maybe_flag = store->GetFlag("flagWithTarget")->item;
    ASSERT_TRUE(maybe_flag);
    data_model::Flag flag = maybe_flag.value();
    ASSERT_FALSE(flag.on);

    auto detail = e.Evaluate(flag, alice);

    ASSERT_TRUE(detail);
    ASSERT_EQ(*detail, Value(false));
    ASSERT_EQ(detail.VariationIndex(), 0);
    ASSERT_EQ(detail.Reason(), EvaluationReason::Off());

    // flip off variation
    flag.offVariation = 1;
    detail = e.Evaluate(flag, alice);
    ASSERT_TRUE(detail);
    ASSERT_EQ(detail.VariationIndex(), 1);
    ASSERT_EQ(*detail, Value(true));

    // off variation unspecified
    flag.offVariation = std::nullopt;
    detail = e.Evaluate(flag, alice);
    ASSERT_TRUE(detail);
    ASSERT_EQ(detail.VariationIndex(), std::nullopt);
    ASSERT_EQ(*detail, Value::Null());

    // flip targeting on
    flag.on = true;
    detail = e.Evaluate(flag, alice);
    ASSERT_TRUE(detail);
    ASSERT_EQ(detail.VariationIndex(), 1);
    ASSERT_EQ(*detail, Value(true));
    ASSERT_EQ(detail.Reason(), EvaluationReason::Fallthrough(false));

    detail = e.Evaluate(flag, bob);
    ASSERT_TRUE(detail);
    ASSERT_EQ(detail.VariationIndex(), 0);
    ASSERT_EQ(*detail, Value(false));
    ASSERT_EQ(detail.Reason(), EvaluationReason::TargetMatch());

    // flip default variation
    flag.fallthrough = data_model::Flag::Variation{0};
    detail = e.Evaluate(flag, alice);
    ASSERT_TRUE(detail);
    ASSERT_EQ(detail.VariationIndex(), 0);
    ASSERT_EQ(*detail, Value(false));

    // bob's reason should still be TargetMatch even though his value is now the
    // default
    detail = e.Evaluate(flag, bob);
    ASSERT_TRUE(detail);
    ASSERT_EQ(detail.VariationIndex(), 0);
    ASSERT_EQ(*detail, Value(false));
    ASSERT_EQ(detail.Reason(), EvaluationReason::TargetMatch());
}

TEST_F(EvaluatorTests, EvaluateWithMatchesOpGroups) {
    evaluation::Evaluator e(logger, *store);

    auto alice = ContextBuilder().Kind("user", "alice").Build();
    auto bob = ContextBuilder()
                   .Kind("user", "bob")
                   .Set("groups", {"my-group"})
                   .Build();

    auto maybe_flag = store->GetFlag("flagWithMatchesOpOnGroups")->item;
    ASSERT_TRUE(maybe_flag);
    data_model::Flag flag = maybe_flag.value();

    auto detail = e.Evaluate(flag, alice);
    ASSERT_TRUE(detail);
    ASSERT_EQ(*detail, Value(true));
    ASSERT_EQ(detail.Reason(), EvaluationReason::Fallthrough(false));

    detail = e.Evaluate(flag, bob);
    ASSERT_TRUE(detail);
    ASSERT_EQ(*detail, Value(false));
    ASSERT_EQ(detail.VariationIndex(), 0);
    ASSERT_EQ(detail.Reason(),
              EvaluationReason::RuleMatch(
                  0, "6a7755ac-e47a-40ea-9579-a09dd5f061bd", false));
}

TEST_F(EvaluatorTests, EvaluateWithMatchesOpKinds) {
    evaluation::Evaluator e(logger, *store);

    auto alice = ContextBuilder().Kind("user", "alice").Build();
    auto bob = ContextBuilder().Kind("company", "bob").Build();

    auto maybe_flag = store->GetFlag("flagWithMatchesOpOnKinds")->item;
    ASSERT_TRUE(maybe_flag);
    data_model::Flag flag = maybe_flag.value();

    auto detail = e.Evaluate(flag, alice);
    ASSERT_TRUE(detail);
    ASSERT_EQ(*detail, Value(false));
    ASSERT_EQ(detail.VariationIndex(), 0);
    ASSERT_EQ(detail.Reason(),
              EvaluationReason::RuleMatch(
                  0, "6a7755ac-e47a-40ea-9579-a09dd5f061bd", false));

    detail = e.Evaluate(flag, bob);
    ASSERT_TRUE(detail);
    ASSERT_EQ(*detail, Value(true));
    ASSERT_EQ(detail.Reason(), EvaluationReason::Fallthrough(false));

    auto new_bob = ContextBuilder().Kind("org", "bob").Build();
    detail = e.Evaluate(flag, new_bob);
    ASSERT_TRUE(detail);
    ASSERT_EQ(*detail, Value(false));
    ASSERT_EQ(detail.VariationIndex(), 0);
    ASSERT_EQ(detail.Reason(),
              EvaluationReason::RuleMatch(
                  0, "6a7755ac-e47a-40ea-9579-a09dd5f061bd", false));
}

TEST_F(EvaluatorTests, PrerequisiteCycle) {
    std::vector<std::string> log_messages;
    auto spy_logger = logging::SpyLogger(log_messages);

    evaluation::Evaluator e(spy_logger, *store);

    auto alice = ContextBuilder().Kind("user", "alice").Build();

    auto maybe_flag = store->GetFlag("cycleFlagA")->item;
    ASSERT_TRUE(maybe_flag);

    auto detail = e.Evaluate(*maybe_flag, alice);
    ASSERT_FALSE(detail);
    ASSERT_EQ(detail.Reason(), EvaluationReason::MalformedFlag());
    ASSERT_EQ(log_messages.size(), 1);
    ASSERT_TRUE(log_messages[0].find("circular reference") !=
                std::string::npos);
}

TEST_F(EvaluatorTests, FlagWithExperiment) {
    evaluation::Evaluator e(logger, *store);

    auto user_a = ContextBuilder().Kind("user", "userKeyA").Build();
    auto user_b = ContextBuilder().Kind("user", "userKeyB").Build();
    auto user_c = ContextBuilder().Kind("user", "userKeyC").Build();

    auto flag = store->GetFlag("flagWithExperiment")->item.value();

    auto detail = e.Evaluate(flag, user_a);
    ASSERT_TRUE(detail);
    ASSERT_EQ(*detail, Value(false));
    ASSERT_TRUE(detail.Reason()->InExperiment());

    detail = e.Evaluate(flag, user_b);
    ASSERT_TRUE(detail);
    ASSERT_EQ(*detail, Value(true));
    ASSERT_TRUE(detail.Reason()->InExperiment());

    detail = e.Evaluate(flag, user_c);
    ASSERT_TRUE(detail);
    ASSERT_EQ(*detail, Value(false));
    ASSERT_FALSE(detail.Reason()->InExperiment());
}

TEST_F(EvaluatorTests, FlagWithExperimentTargetingMissingContext) {
    evaluation::Evaluator e(logger, *store);

    auto flag =
        store->GetFlag("flagWithExperimentTargetingContext")->item.value();

    auto user_a = ContextBuilder().Kind("user", "userKeyA").Build();

    auto detail = e.Evaluate(flag, user_a);
    ASSERT_TRUE(detail);
    ASSERT_EQ(*detail, Value(false));
    ASSERT_EQ(detail.Reason(), EvaluationReason::Fallthrough(false));
}
