#include <launchdarkly/data_model/flag.hpp>

namespace launchdarkly::data_model {

Flag::Rollout::WeightedVariation::WeightedVariation(Flag::Variation variation_,
                                                    Flag::Weight weight_)
    : WeightedVariation(variation_, weight_, false) {}

Flag::Rollout::WeightedVariation::WeightedVariation(Flag::Variation variation_,
                                                    Flag::Weight weight_,
                                                    bool untracked_)
    : variation(variation_), weight(weight_), untracked(untracked_) {}

Flag::Rollout::WeightedVariation Flag::Rollout::WeightedVariation::Untracked(
    Flag::Variation variation_,
    Flag::Weight weight_) {
    return {variation_, weight_, true};
}
Flag::Rollout::WeightedVariation::WeightedVariation()
    : variation(0), weight(0), untracked(false) {}

Flag::Rollout::Rollout(std::vector<WeightedVariation> variations_)
    : variations(std::move(variations_)),
      kind(Kind::kRollout),
      seed(std::nullopt),
      bucketBy("key"),
      contextKind("user") {}

bool Flag::IsExperimentationEnabled(
    std::optional<EvaluationReason> const& reason) const {
    if (!reason) {
        return false;
    }
    if (reason->InExperiment()) {
        return true;
    }
    switch (reason->Kind()) {
        case EvaluationReason::Kind::kFallthrough:
            return this->trackEventsFallthrough;
        case EvaluationReason::Kind::kRuleMatch:
            if (!reason->RuleIndex() ||
                reason->RuleIndex() >= this->rules.size()) {
                return false;
            }
            return this->rules.at(*reason->RuleIndex()).trackEvents;
        default:
            return false;
    }
}

}  // namespace launchdarkly::data_model
