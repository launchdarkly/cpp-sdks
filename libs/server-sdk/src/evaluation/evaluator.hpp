#pragma once

#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_detail.hpp>
#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/value.hpp>

#include "../flag_manager/flag_store.hpp"
#include "bucketing.hpp"
#include "detail/evaluation_stack.hpp"
#include "evaluation_error.hpp"

#include <tl/expected.hpp>

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

tl::expected<BucketResult, Error> Variation(
    data_model::Flag::VariationOrRollout const& vr,
    std::string const& flag_key,
    launchdarkly::Context const& context,
    std::string const& salt);

}  // namespace launchdarkly::server_side::evaluation
