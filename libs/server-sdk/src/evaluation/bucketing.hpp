#pragma once

#include "evaluation_error.hpp"

#include <launchdarkly/attribute_reference.hpp>
#include <launchdarkly/context.hpp>
#include <launchdarkly/data_model/flag.hpp>

#include <tl/expected.hpp>

#include <limits>
#include <optional>
#include <ostream>
#include <string>
#include <variant>

namespace launchdarkly::server_side::evaluation {

double const kBucketScale = 100'000.0;

enum RolloutKindLookup {
    /* The rollout's context kind was found in the supplied evaluation context.
     */
    kPresent,
    /* The rollout's context kind was not found in the supplied evaluation
     * context. */
    kAbsent
};

/**
 * Bucketing is performed by hashing an input string. This string
 * may be comprised of a seed (if the flag rule has a seed) or a combined
 * key/salt pair.
 */
class BucketPrefix {
   public:
    struct KeyAndSalt {
        std::string key;
        std::string salt;
    };

    using Seed = std::int64_t;

    /**
     * Constructs a BucketPrefix from a seed value.
     * @param seed Value of the seed.
     */
    explicit BucketPrefix(Seed seed);

    /**
     * Constructs a BucketPrefix from a key and salt.
     * @param key Key to use.
     * @param salt Salt to use.
     */
    BucketPrefix(std::string key, std::string salt);

    friend std::ostream& operator<<(std::ostream& os,
                                    BucketPrefix const& prefix);

   private:
    std::variant<KeyAndSalt, Seed> prefix_;
};

using ContextHashValue = float;

/**
 * Computes the context hash value for an attribute in the given context
 * identified by the given attribute reference. The hash value is
 * augmented with the supplied bucket prefix.
 *
 * @param context Context to query.
 * @param by_attr Identifier of the attribute to hash. If is_experiment is true,
 * then "key" will be used regardless of by_attr's value.
 * @param prefix Prefix to use when hashing.
 * @param is_experiment Whether this rollout is an experiment.
 * @param context_kind Which kind to inspect in the context.
 * @return A context hash value and indication of whether or not context_kind
 * was found in the context.
 */
tl::expected<std::pair<ContextHashValue, RolloutKindLookup>, Error> Bucket(
    Context const& context,
    AttributeReference const& by_attr,
    BucketPrefix const& prefix,
    bool is_experiment,
    std::string const& context_kind);

class BucketResult {
   public:
    BucketResult(
        data_model::Flag::Rollout::WeightedVariation weighted_variation,
        bool is_experiment);

    BucketResult(data_model::Flag::Variation variation, bool in_experiment);

    BucketResult(data_model::Flag::Variation variation);

    [[nodiscard]] std::size_t VariationIndex() const;

    [[nodiscard]] bool InExperiment() const;

   private:
    std::size_t variation_index_;
    bool in_experiment_;
};

/**
 * Given a variation or rollout and associated flag key, computes the proper
 * variation index for the context. For a plain variation, this is simply the
 * variation index. For a rollout, this utilizes the Bucket function.
 *
 * @param vr Variation or rollout.
 * @param flag_key Key of flag.
 * @param context Context to bucket.
 * @param salt Salt to use when bucketing.
 * @return A BucketResult on success, or an error if bucketing failed.
 */
tl::expected<BucketResult, Error> Variation(
    data_model::Flag::VariationOrRollout const& vr,
    std::string const& flag_key,
    launchdarkly::Context const& context,
    std::optional<std::string> const& salt);

}  // namespace launchdarkly::server_side::evaluation
