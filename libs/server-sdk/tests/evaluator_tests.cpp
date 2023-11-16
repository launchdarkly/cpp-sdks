#include "evaluation/evaluator.hpp"
#include "test_store.hpp"

#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/logging/null_logger.hpp>

#include "spy_logger.hpp"

#include <gtest/gtest.h>

using namespace launchdarkly;
using namespace launchdarkly::server_side;

/**
 * Use if the test does not require inspecting log messages.
 */
class EvaluatorTests : public ::testing::Test {
   public:
    EvaluatorTests()
        : logger_(logging::NullLogger()),
          store_(test_store::TestData()),
          eval_(logger_, *store_) {}

   private:
    Logger logger_;

   protected:
    std::unique_ptr<data_interfaces::IStore const> store_;
    evaluation::Evaluator eval_;
};

/**
 * Use if the test requires making assertions based on log messages generated
 * during evaluation.
 */
class EvaluatorTestsWithLogs : public ::testing::Test {
   public:
    EvaluatorTestsWithLogs()
        : messages_(std::make_shared<logging::SpyLoggerBackend>()),
          logger_(messages_),
          store_(test_store::TestData()),
          eval_(logger_, *store_) {}

   protected:
    std::shared_ptr<logging::SpyLoggerBackend> messages_;

   private:
    Logger logger_;

   protected:
    std::unique_ptr<data_interfaces::IStore const> store_;
    evaluation::Evaluator eval_;
};

TEST_F(EvaluatorTests, BasicChanges) {
    auto alice = ContextBuilder().Kind("user", "alice").Build();
    auto bob = ContextBuilder().Kind("user", "bob").Build();

    auto flag = store_->GetFlag("flagWithTarget")->item.value();

    ASSERT_FALSE(flag.on);

    auto detail = eval_.Evaluate(flag, alice);

    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(*detail, Value(false));
    ASSERT_EQ(detail.VariationIndex(), 0);
    ASSERT_EQ(detail.Reason(), EvaluationReason::Off());

    // flip off variation
    flag.offVariation = 1;
    detail = eval_.Evaluate(flag, alice);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(detail.VariationIndex(), 1);
    ASSERT_EQ(*detail, Value(true));

    // off variation unspecified
    flag.offVariation = std::nullopt;
    detail = eval_.Evaluate(flag, alice);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(detail.VariationIndex(), std::nullopt);
    ASSERT_EQ(*detail, Value::Null());

    // flip targeting on
    flag.on = true;
    detail = eval_.Evaluate(flag, alice);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(detail.VariationIndex(), 1);
    ASSERT_EQ(*detail, Value(true));
    ASSERT_EQ(detail.Reason(), EvaluationReason::Fallthrough(false));

    detail = eval_.Evaluate(flag, bob);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(detail.VariationIndex(), 0);
    ASSERT_EQ(*detail, Value(false));
    ASSERT_EQ(detail.Reason(), EvaluationReason::TargetMatch());

    // flip default variation
    flag.fallthrough = data_model::Flag::Variation{0};
    detail = eval_.Evaluate(flag, alice);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(detail.VariationIndex(), 0);
    ASSERT_EQ(*detail, Value(false));

    // bob's reason should still be TargetMatch even though his value is now the
    // default
    detail = eval_.Evaluate(flag, bob);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(detail.VariationIndex(), 0);
    ASSERT_EQ(*detail, Value(false));
    ASSERT_EQ(detail.Reason(), EvaluationReason::TargetMatch());
}

TEST_F(EvaluatorTests, EvaluateWithMatchesOpGroups) {
    auto alice = ContextBuilder().Kind("user", "alice").Build();
    auto bob = ContextBuilder()
                   .Kind("user", "bob")
                   .Set("groups", {"my-group"})
                   .Build();

    auto flag = store_->GetFlag("flagWithMatchesOpOnGroups")->item.value();

    auto detail = eval_.Evaluate(flag, alice);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(*detail, Value(true));
    ASSERT_EQ(detail.Reason(), EvaluationReason::Fallthrough(false));

    detail = eval_.Evaluate(flag, bob);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(*detail, Value(false));
    ASSERT_EQ(detail.VariationIndex(), 0);
    ASSERT_EQ(detail.Reason(),
              EvaluationReason::RuleMatch(
                  0, "6a7755ac-e47a-40ea-9579-a09dd5f061bd", false));
}

TEST_F(EvaluatorTests, EvaluateWithMatchesOpKinds) {
    auto alice = ContextBuilder().Kind("user", "alice").Build();
    auto bob = ContextBuilder().Kind("company", "bob").Build();

    auto flag = store_->GetFlag("flagWithMatchesOpOnKinds")->item.value();

    auto detail = eval_.Evaluate(flag, alice);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(*detail, Value(false));
    ASSERT_EQ(detail.VariationIndex(), 0);
    ASSERT_EQ(detail.Reason(),
              EvaluationReason::RuleMatch(
                  0, "6a7755ac-e47a-40ea-9579-a09dd5f061bd", false));

    detail = eval_.Evaluate(flag, bob);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(*detail, Value(true));
    ASSERT_EQ(detail.Reason(), EvaluationReason::Fallthrough(false));

    auto new_bob = ContextBuilder().Kind("org", "bob").Build();
    detail = eval_.Evaluate(flag, new_bob);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(*detail, Value(false));
    ASSERT_EQ(detail.VariationIndex(), 0);
    ASSERT_EQ(detail.Reason(),
              EvaluationReason::RuleMatch(
                  0, "6a7755ac-e47a-40ea-9579-a09dd5f061bd", false));
}

TEST_F(EvaluatorTestsWithLogs, PrerequisiteCycle) {
    auto alice = ContextBuilder().Kind("user", "alice").Build();

    auto flag = store_->GetFlag("cycleFlagA")->item.value();

    auto detail = eval_.Evaluate(flag, alice);
    ASSERT_TRUE(detail.IsError());
    ASSERT_EQ(detail.Reason(), EvaluationReason::MalformedFlag());
    ASSERT_TRUE(messages_->Count(1));
    ASSERT_TRUE(messages_->Contains(0, LogLevel::kError, "circular reference"));
}

TEST_F(EvaluatorTests, FlagWithSegment) {
    auto alice = ContextBuilder().Kind("user", "alice").Build();
    auto bob = ContextBuilder().Kind("user", "bob").Build();

    auto flag = store_->GetFlag("flagWithSegmentMatchRule")->item.value();

    auto detail = eval_.Evaluate(flag, alice);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(*detail, Value(false));
    ASSERT_EQ(detail.Reason(),
              EvaluationReason::RuleMatch(0, "match-rule", false));

    detail = eval_.Evaluate(flag, bob);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(*detail, Value(true));
    ASSERT_EQ(detail.Reason(), EvaluationReason::Fallthrough(false));
}

TEST_F(EvaluatorTests, FlagWithSegmentContainingRules) {
    auto alice = ContextBuilder().Kind("user", "alice").Build();
    auto bob = ContextBuilder().Kind("user", "bob").Build();

    auto flag =
        store_->GetFlag("flagWithSegmentMatchesUserAlice")->item.value();

    auto detail = eval_.Evaluate(flag, alice);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(detail.Reason(),
              EvaluationReason::RuleMatch(0, "match-rule", false));
    ASSERT_EQ(*detail, Value(false));

    detail = eval_.Evaluate(flag, bob);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(detail.Reason(), EvaluationReason::Fallthrough(false));
    ASSERT_EQ(*detail, Value(true));
}

TEST_F(EvaluatorTests, FlagWithExperiment) {
    auto user_a = ContextBuilder().Kind("user", "userKeyA").Build();
    auto user_b = ContextBuilder().Kind("user", "userKeyB").Build();
    auto user_c = ContextBuilder().Kind("user", "userKeyC").Build();

    auto flag = store_->GetFlag("flagWithExperiment")->item.value();

    auto detail = eval_.Evaluate(flag, user_a);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(*detail, Value(false));
    ASSERT_TRUE(detail.Reason()->InExperiment());

    detail = eval_.Evaluate(flag, user_b);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(*detail, Value(true));
    ASSERT_TRUE(detail.Reason()->InExperiment());

    detail = eval_.Evaluate(flag, user_c);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(*detail, Value(false));
    ASSERT_FALSE(detail.Reason()->InExperiment());
}

TEST_F(EvaluatorTests, FlagWithExperimentTargetingMissingContext) {
    auto flag =
        store_->GetFlag("flagWithExperimentTargetingContext")->item.value();

    auto user_a = ContextBuilder().Kind("user", "userKeyA").Build();

    auto detail = eval_.Evaluate(flag, user_a);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(*detail, Value(false));
    ASSERT_EQ(detail.Reason(), EvaluationReason::Fallthrough(false));
}
