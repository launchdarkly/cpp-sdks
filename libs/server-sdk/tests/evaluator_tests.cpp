#include <gtest/gtest.h>
#include <launchdarkly/logging/null_logger.hpp>

#include "evaluation/evaluator.hpp"
#include "flag_manager/flag_store.hpp"

#include <launchdarkly/context.hpp>
#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/data_model/flag.hpp>

using namespace launchdarkly;
using namespace launchdarkly::server_side;

TEST(EvaluatorTests, Instantiation) {
    auto logger = logging::NullLogger();

    flag_manager::FlagStore store;

    evaluation::Evaluator e(logger, store);

    auto result = e.Evaluate(data_model::Flag(),
                             ContextBuilder().Kind("cat", "shadow").Build());

    ASSERT_TRUE((*result).IsNull());
    ASSERT_EQ(result.Reason(), EvaluationReason::Off());
}
