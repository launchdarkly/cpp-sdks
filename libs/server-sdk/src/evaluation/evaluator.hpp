#pragma once

#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_detail.hpp>
#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/logging/logger.hpp>

#include <launchdarkly/value.hpp>

#include "detail/evaluation_stack.hpp"

namespace launchdarkly::evaluation {

class Evaluator {
   public:
    Evaluator(Logger& logger);
    EvaluationDetail<Value> Evaluate(data_model::Flag& flag,
                                     launchdarkly::Context& context);

   private:
    Logger& logger_;
    detail::EvaluationStack stack_;
};

}  // namespace launchdarkly::evaluation
