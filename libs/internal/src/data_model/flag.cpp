#include <launchdarkly/data_model/flag.hpp>

namespace launchdarkly::data_model {

Flag::Rollout::WeightedVariation::WeightedVariation(std::uint64_t variation_,
                                                    std::uint64_t weight_)
    : variation(variation_), weight(weight_), untracked(false) {}

Flag::Rollout::Rollout(std::vector<WeightedVariation> variations_)
    : variations(std::move(variations_)),
      bucketBy("key"),
      contextKind("user"),
      kind(Kind::kRollout),
      seed(std::nullopt) {}

}  // namespace launchdarkly::data_model
