#include <launchdarkly/data_model/flag.hpp>

namespace launchdarkly::data_model {

Flag::Rollout::WeightedVariation::WeightedVariation(std::uint64_t variation_,
                                                    std::uint64_t weight_)
    : WeightedVariation(variation_, weight_, false) {}

Flag::Rollout::WeightedVariation::WeightedVariation(std::uint64_t variation_,
                                                    std::uint64_t weight_,
                                                    bool untracked_)
    : variation(variation_), weight(weight_), untracked(untracked_) {}

Flag::Rollout::WeightedVariation Flag::Rollout::WeightedVariation::Untracked(
    Flag::Variation variation_,
    std::uint64_t weight_) {
    return {variation_, weight_, true};
}

Flag::Rollout::Rollout(std::vector<WeightedVariation> variations_)
    : variations(std::move(variations_)),
      bucketBy("key"),
      contextKind("user"),
      kind(Kind::kRollout),
      seed(std::nullopt) {}

}  // namespace launchdarkly::data_model
