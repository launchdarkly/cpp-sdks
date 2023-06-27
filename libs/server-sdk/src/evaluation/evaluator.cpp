#include "evaluator.hpp"

#include <launchdarkly/data/evaluation_detail.hpp>
#include <launchdarkly/data/evaluation_reason.hpp>

#include <launchdarkly/value.hpp>

namespace launchdarkly::evaluation {

EvaluationDetail<Value> off_value(data_model::Flag const& flag,
                                  EvaluationReason reason) {
    if (flag.offVariation)
}

Evaluator::Evaluator() : stack_(20) {}

void Evaluator::Evaluate(data_model::Flag& flag,
                         launchdarkly::Context& context) {
    if (!flag.on) {
        return flag.off
    }
}

}  // namespace launchdarkly::evaluation
