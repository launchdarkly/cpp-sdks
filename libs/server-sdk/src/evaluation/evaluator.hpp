#pragma once

#include <launchdarkly/context.hpp>
#include <launchdarkly/data_model/flag.hpp>
#include "detail/evaluation_stack.hpp"

namespace launchdarkly::evaluation {

class Evaluator {
   public:
    Evaluator();
    void Evaluate(data_model::Flag const& flag, Context const& context);

   private:
    detail::EvaluationStack stack_;
};

}  // namespace launchdarkly::evaluation
