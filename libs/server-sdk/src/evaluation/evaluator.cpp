#include "evaluator.hpp"
#include "bucketing.hpp"
#include "operators.hpp"
#include "rules.hpp"

#include <launchdarkly/data/evaluation_reason.hpp>

#include <tl/expected.hpp>

#include <algorithm>
#include <variant>

namespace launchdarkly::server_side::evaluation {

EvaluationDetail<Value> FlagVariation(data_model::Flag const& flag,
                                      std::uint64_t variation_index,
                                      EvaluationReason reason);

EvaluationDetail<Value> OffValue(data_model::Flag const& flag,
                                 EvaluationReason reason);

tl::expected<std::optional<BucketResult>, Error> Variation(
    data_model::Flag::VariationOrRollout const& vr,
    std::string const& flag_key,
    Context const& context,
    std::string const& salt);

tl::expected<BucketResult, enum EvaluationReason::ErrorKind>
ResolveVariationOrRollout(data_model::Flag const& flag,
                          data_model::Flag::VariationOrRollout const& vr,
                          Context const& context);

std::optional<std::size_t> AnyTargetMatchVariation(
    launchdarkly::Context const& context,
    data_model::Flag const& flag);

std::optional<std::size_t> TargetMatchVariation(
    launchdarkly::Context const& context,
    data_model::Flag::Target const& target);

Evaluator::Evaluator(Logger& logger, flag_manager::FlagStore const& store)
    : logger_(logger), store_(store), stack_(20) {}

EvaluationDetail<Value> Evaluator::Evaluate(
    data_model::Flag const& flag,
    launchdarkly::Context const& context) {
    // If the flag is off, return immediately.
    if (!flag.on) {
        return OffValue(flag, EvaluationReason::Off());
    }

    // If this flag has already been seen in this branch of recursion, then
    // it must be a circular reference.
    if (stack_.SeenPrerequisite(flag.key)) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "prerequisite relationship to " << flag.key
            << " caused a circular reference; this is probably a temporary "
               "condition due to an incomplete update";
        return OffValue(flag, EvaluationReason(
                                  EvaluationReason::ErrorKind::kMalformedFlag));
    }

    {
        // Visit all prerequisites. Each prerequisite evaluation has an
        // opportunity to influence the evaluation result of the original
        // flag.

        auto guard = stack_.NoticePrerequisite(flag.key);

        for (data_model::Flag::Prerequisite const& prereq :
             flag.prerequisites) {
            flag_manager::FlagStore::FlagItem prereq_flag =
                store_.GetFlag(prereq.key);
            if (!prereq_flag) {
                return OffValue(
                    flag, EvaluationReason::PrerequisiteFailed(prereq.key));
            }

            flag_manager::FlagItemDescriptor const& prereq_flag_ref =
                *prereq_flag;

            if (!prereq_flag_ref.item) {
                // This flag existed at some point, but has since been deleted.
                return OffValue(
                    flag, EvaluationReason::PrerequisiteFailed(prereq.key));
            }

            if (stack_.SeenPrerequisite(prereq_flag->item->key)) {
                return EvaluationDetail<Value>(
                    EvaluationReason::ErrorKind::kMalformedFlag, Value());
            }

            EvaluationDetail<Value> prerequisite_result =
                Evaluate(*prereq_flag_ref.item, context);

            if (prerequisite_result.Reason().has_value() &&
                prerequisite_result.Reason().value().Kind() ==
                    EvaluationReason::Kind::kError) {
                return EvaluationDetail<Value>(
                    EvaluationReason::ErrorKind::kMalformedFlag, Value());
            }

            std::optional<std::size_t> variation_index =
                prerequisite_result.VariationIndex();

            // todo(cwaldren) record prerequisite evaluation

            // Although it would seem we could immediately fail the prereq if it
            // is off (after retrieving it from the store) that needs to be
            // deferred until this point, in order to generate prerequisite
            // events.

            if (!prereq_flag_ref.item->on ||
                variation_index != prereq.variation) {
                return OffValue(
                    flag, EvaluationReason::PrerequisiteFailed(prereq.key));
            }
        }
    }

    // If the flag is on, all prerequisites are on and valid, then
    // determine if the context matches any targets. This happens before
    // rule evaluation to ensure targets always have priority.

    if (std::optional<std::size_t> variation_index =
            AnyTargetMatchVariation(context, flag)) {
        return FlagVariation(flag, *variation_index,
                             EvaluationReason::TargetMatch());
    }

    // If there were no target matches, then evaluate against the
    // flag's rules.

    for (std::size_t i = 0; i < flag.rules.size(); i++) {
        auto const& rule = flag.rules[i];
        tl::expected<bool, enum EvaluationReason::ErrorKind> rule_match =
            Match(rule, context, store_, stack_);

        if (!rule_match) {
            LD_LOG(logger_, LogLevel::kWarn) << rule_match.error();
            return EvaluationDetail<Value>(
                EvaluationReason::ErrorKind::kMalformedFlag, Value());
        }
        if (!rule_match.value()) {
            continue;
        }
        auto result =
            ResolveVariationOrRollout(flag, rule.variationOrRollout, context);
        if (!result) {
            return EvaluationDetail<Value>(result.error(), Value());
        }
        auto reason =
            EvaluationReason::RuleMatch(i, rule.id, result->in_experiment);
        return FlagVariation(flag, result->variation_index, std::move(reason));
    }

    // If there were no rule matches, then return the fallthrough variation.

    auto result = ResolveVariationOrRollout(flag, flag.fallthrough, context);
    if (!result) {
        return EvaluationDetail<Value>(result.error(), Value());
    }
    auto reason = EvaluationReason::Fallthrough(result->in_experiment);
    return FlagVariation(flag, result->variation_index, std::move(reason));
}

EvaluationDetail<Value> FlagVariation(data_model::Flag const& flag,
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
        return FlagVariation(flag, *flag.offVariation, std::move(reason));
    }
    return EvaluationDetail<Value>(std::move(reason));
}

std::optional<std::size_t> AnyTargetMatchVariation(
    launchdarkly::Context const& context,
    data_model::Flag const& flag) {
    if (flag.contextTargets.empty()) {
        for (auto const& target : flag.targets) {
            if (auto index = TargetMatchVariation(context, target)) {
                return index;
            }
        }
    } else {
        for (auto const& context_target : flag.contextTargets) {
            if (context_target.contextKind == "user" &&
                context_target.values.empty()) {
                for (auto const& target : flag.targets) {
                    if (target.variation == context_target.variation) {
                        if (auto index =
                                TargetMatchVariation(context, target)) {
                            return index;
                        }
                    }
                }
            } else if (auto index =
                           TargetMatchVariation(context, context_target)) {
                return index;
            }
        }
    }

    return std::nullopt;
}

std::optional<std::size_t> TargetMatchVariation(
    launchdarkly::Context const& context,
    data_model::Flag::Target const& target) {
    Value key = context.Get(target.contextKind, "key");
    if (!key) {
        return std::nullopt;
    }

    for (auto const& value : target.values) {
        if (value == key) {
            return target.variation;
        }
    }

    return std::nullopt;
}

tl::expected<std::optional<BucketResult>, Error> Variation(
    data_model::Flag::VariationOrRollout const& vr,
    std::string const& flag_key,
    Context const& context,
    std::string const& salt) {
    return std::visit(
        [&](auto&& arg) -> tl::expected<std::optional<BucketResult>, Error> {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, data_model::Flag::Variation>) {
                return BucketResult(arg, false);
            } else if constexpr (std::is_same_v<T, data_model::Flag::Rollout>) {
                if (arg.variations.empty()) {
                    return tl::make_unexpected(
                        Error::kRolloutMissingVariations);
                }

                bool is_experiment =
                    arg.kind == data_model::Flag::Rollout::Kind::kExperiment;

                std::optional<BucketPrefix> prefix;
                if (arg.seed) {
                    prefix = BucketPrefix(*arg.seed);
                } else {
                    prefix = BucketPrefix(flag_key, salt);
                }

                auto bucketing_result = Bucket(context, arg.bucketBy, *prefix,
                                               is_experiment, arg.contextKind);
                if (!bucketing_result) {
                    return tl::make_unexpected(bucketing_result.error());
                }
                auto [bucket, lookup] = *bucketing_result;

                double sum = 0.0;

                for (const auto& variation : arg.variations) {
                    sum += variation.weight / 100000.0;
                    if (bucket < sum) {
                        return BucketResult(
                            variation,
                            is_experiment &&
                                lookup == RolloutKindPresence::kPresent);
                    }
                }

                return BucketResult(
                    arg.variations.back(),
                    is_experiment && lookup == RolloutKindPresence::kPresent);
            }
        },
        vr);
}
tl::expected<BucketResult, enum EvaluationReason::ErrorKind>
ResolveVariationOrRollout(data_model::Flag const& flag,
                          data_model::Flag::VariationOrRollout const& vr,
                          Context const& context) {
    if (!flag.salt) {
        return tl::make_unexpected(EvaluationReason::ErrorKind::kMalformedFlag);
    }

    const tl::expected<std::optional<BucketResult>, Error> result =
        Variation(vr, flag.key, context, *flag.salt);

    if (!result) {
        return tl::make_unexpected(EvaluationReason::ErrorKind::kMalformedFlag);
    }
    auto const& bucket = result.value();
    if (!bucket) {
        return tl::make_unexpected(EvaluationReason::ErrorKind::kMalformedFlag);
    }
    return *bucket;
}

}  // namespace launchdarkly::server_side::evaluation
