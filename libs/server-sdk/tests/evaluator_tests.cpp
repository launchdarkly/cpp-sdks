#include <gtest/gtest.h>
#include <launchdarkly/logging/null_logger.hpp>

#include "evaluation/evaluator.hpp"
#include "test_store.hpp"

#include <launchdarkly/context.hpp>
#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/data_model/flag.hpp>

using namespace launchdarkly;
using namespace launchdarkly::server_side;

TEST(EvaluatorTests, Instantiation) {
    auto logger = logging::NullLogger();

    auto store = test_store::Make();

    evaluation::Evaluator e(logger, store.get());

    auto alice = ContextBuilder().Kind("user", "alice").Build();
    auto bob = ContextBuilder().Kind("user", "bob").Build();

    auto maybe_flag = store->GetFlag("flagWithTarget")->item;
    ASSERT_TRUE(maybe_flag);
    data_model::Flag flag = maybe_flag.value();
    ASSERT_FALSE(flag.on);

    auto detail = e.Evaluate(flag, alice);

    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(*detail, Value(false));
    ASSERT_EQ(detail.VariationIndex(), 0);
    ASSERT_EQ(detail.Reason()->Kind(), EvaluationReason::Kind::kOff);

    // flip off variation
    flag.offVariation = 1;
    detail = e.Evaluate(flag, alice);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(detail.VariationIndex(), 1);
    ASSERT_EQ(*detail, Value(true));

    // off variation unspecified
    flag.offVariation = std::nullopt;
    detail = e.Evaluate(flag, alice);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(detail.VariationIndex(), std::nullopt);
    ASSERT_EQ(*detail, Value::Null());

    // flip targeting on
    flag.on = true;
    detail = e.Evaluate(flag, alice);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(detail.VariationIndex(), 1);
    ASSERT_EQ(*detail, Value(true));
    ASSERT_EQ(detail.Reason()->Kind(), EvaluationReason::Kind::kFallthrough);
    ASSERT_FALSE(detail.Reason()->InExperiment());

    detail = e.Evaluate(flag, bob);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(detail.VariationIndex(), 0);
    ASSERT_EQ(*detail, Value(false));
    ASSERT_EQ(detail.Reason()->Kind(), EvaluationReason::Kind::kTargetMatch);

    // flip default variation
    flag.fallthrough = data_model::Flag::Variation{0};
    detail = e.Evaluate(flag, alice);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(detail.VariationIndex(), 0);
    ASSERT_EQ(*detail, Value(false));

    // bob's reason should still be TargetMatch even though his value is now the
    // default
    detail = e.Evaluate(flag, bob);
    ASSERT_FALSE(detail.IsError());
    ASSERT_EQ(detail.VariationIndex(), 0);
    ASSERT_EQ(*detail, Value(false));
    ASSERT_EQ(detail.Reason()->Kind(), EvaluationReason::Kind::kTargetMatch);
}
