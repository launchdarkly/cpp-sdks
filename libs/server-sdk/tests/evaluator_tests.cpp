#include <gtest/gtest.h>
#include <launchdarkly/logging/null_logger.hpp>

#include "evaluation/evaluator.hpp"

using namespace launchdarkly::evaluation;
TEST(EvaluatorTests, Instantiation) {
    auto logger = launchdarkly::logging::NullLogger();

    Evaluator e(logger);
}
