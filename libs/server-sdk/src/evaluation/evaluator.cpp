#include "evaluator.hpp"

#include <launchdarkly/data/evaluation_detail.hpp>
#include <launchdarkly/data/evaluation_reason.hpp>

#include <launchdarkly/value.hpp>

namespace launchdarkly::evaluation {

EvaluationDetail<Value> Variation(data_model::Flag const& flag,
                                  std::uint64_t variation_index,
                                  EvaluationReason reason) {
    if (variation_index >= flag.variations.size()) {
        return EvaluationDetail<Value>(
            EvaluationReason::ErrorKind::kMalformedFlag, Value());
    }
    auto const& variation = flag.variations.at(variation_index);
    return EvaluationDetail<Value>(variation, variation_index,
                                   std::move(reason));
}

EvaluationDetail<Value> OffValue(data_model::Flag const& flag,
                                 EvaluationReason reason) {
    if (flag.offVariation) {
        return Variation(flag, *flag.offVariation, std::move(reason));
    }
    return EvaluationDetail<Value>(std::move(reason));
}

Evaluator::Evaluator(Logger& logger) : logger_(logger), stack_(20) {}

EvaluationDetail<Value> Evaluator::Evaluate(data_model::Flag& flag,
                                            launchdarkly::Context& context) {
    if (!flag.on) {
        return OffValue(flag, EvaluationReason::Off());
    }

    if (stack_.SeenPrerequisite(flag.key)) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "prerequisite relationship to " << flag.key
            << " caused a circular reference; this is probably a temporary "
               "condition due to an incomplete update";
        return OffValue(flag, EvaluationReason(
                                  EvaluationReason::ErrorKind::kMalformedFlag));
    }
}

}  // namespace launchdarkly::evaluation
