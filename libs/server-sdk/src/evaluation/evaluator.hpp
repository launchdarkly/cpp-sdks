#pragma once

#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_detail.hpp>
#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/value.hpp>

#include "../flag_manager/flag_store.hpp"
#include "detail/evaluation_stack.hpp"
#include "evaluation_error.hpp"

namespace launchdarkly::server_side::evaluation {

class Evaluator {
   public:
    Evaluator(Logger& logger, flag_manager::FlagStore const& store);
    [[nodiscard]] EvaluationDetail<Value> Evaluate(
        data_model::Flag const& flag,
        launchdarkly::Context const& context);

   private:
    Logger& logger_;
    flag_manager::FlagStore const& store_;
    mutable detail::EvaluationStack stack_;
};

}  // namespace launchdarkly::server_side::evaluation
