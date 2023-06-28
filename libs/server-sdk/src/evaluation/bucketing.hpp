#pragma once
#include <launchdarkly/attribute_reference.hpp>
#include <launchdarkly/context.hpp>
#include <launchdarkly/data_model/flag.hpp>
#include <limits>
#include <optional>
#include <string>
#include <tl/expected.hpp>
#include <variant>

namespace launchdarkly::server_side::evaluation {

enum RolloutKindPresence {
    /* The rollout's contextKind was found in the current evaluation context. */
    kPresent,
    /* The rollout's contextKind was not found in the current evaluation
     * context. */
    kAbsent
};

class BucketPrefix {
   public:
    struct KeyAndSalt {
        std::string key;
        std::string salt;
    };
    using Seed = std::int64_t;
    BucketPrefix(Seed seed) : prefix_(seed) {}
    BucketPrefix(std::string key, std::string salt)
        : prefix_(KeyAndSalt{key, salt}) {}

    void Update(std::string* input) const {
        return std::visit(
            [&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, KeyAndSalt>) {
                    input->append(arg.key);
                    input->push_back('.');
                    input->append(arg.salt);
                } else if constexpr (std::is_same_v<T, Seed>) {
                    input->append(std::to_string(arg));
                }
            },
            prefix_);
    }

   private:
    std::variant<KeyAndSalt, Seed> prefix_;
};

struct BucketResult {
    std::size_t variation_index;
    bool in_experiment;

    BucketResult(
        data_model::Flag::Rollout::WeightedVariation weighted_variation,
        bool is_experiment)
        : variation_index(weighted_variation.variation),
          in_experiment(is_experiment && !weighted_variation.untracked) {}

    BucketResult(data_model::Flag::Variation variation, bool in_experiment)
        : variation_index(variation), in_experiment(in_experiment) {}
};

std::optional<float> ComputeBucket(Value const& value,
                                   BucketPrefix prefix,
                                   bool is_experiment);

std::optional<std::string> BucketValue(Value const& value);

using ContextHashValue = float;

tl::expected<std::pair<ContextHashValue, RolloutKindPresence>, char const*>
Bucket(Context const& context,
       AttributeReference const& by_attr,
       BucketPrefix const& prefix,
       bool is_experiment,
       std::string const& context_kind);

}  // namespace launchdarkly::server_side::evaluation
