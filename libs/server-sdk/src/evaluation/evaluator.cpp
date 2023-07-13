#include "evaluator.hpp"
#include "rules.hpp"

#include <boost/core/ignore_unused.hpp>
#include <tl/expected.hpp>

#include <algorithm>
#include <variant>

namespace launchdarkly::server_side::evaluation {

using namespace data_model;

BucketResult::BucketResult(
    data_model::Flag::Rollout::WeightedVariation weighted_variation,
    bool is_experiment)
    : variation_index_(weighted_variation.variation),
      in_experiment_(is_experiment && !weighted_variation.untracked) {}

BucketResult::BucketResult(data_model::Flag::Variation variation,
                           bool in_experiment)
    : variation_index_(variation), in_experiment_(in_experiment) {}

BucketResult::BucketResult(data_model::Flag::Variation variation)
    : variation_index_(variation), in_experiment_(false) {}

std::size_t BucketResult::VariationIndex() const {
    return variation_index_;
}

bool BucketResult::InExperiment() const {
    return in_experiment_;
}

EvaluationDetail<Value> OffValue(Flag const& flag, EvaluationReason reason);

tl::expected<BucketResult, enum EvaluationReason::ErrorKind>
ResolveVariationOrRollout(Flag const& flag,
                          Flag::VariationOrRollout const& vr,
                          Context const& context);

std::optional<std::size_t> AnyTargetMatchVariation(
    launchdarkly::Context const& context,
    Flag const& flag);

std::optional<std::size_t> TargetMatchVariation(
    launchdarkly::Context const& context,
    Flag::Target const& target);

Evaluator::Evaluator(Logger& logger, flag_manager::FlagStore const& store)
    : logger_(logger), store_(store), stack_() {}

EvaluationDetail<Value> Evaluator::Evaluate(
    Flag const& flag,
    launchdarkly::Context const& context) {
    return Evaluate("", flag, context);
}

EvaluationDetail<Value> Evaluator::Evaluate(
    std::string const& parent_key,
    Flag const& flag,
    launchdarkly::Context const& context) {
    if (auto guard = stack_.NoticePrerequisite(flag.key)) {
        if (!flag.on) {
            return OffValue(flag, EvaluationReason::Off());
        }

        for (Flag::Prerequisite const& p : flag.prerequisites) {
            flag_manager::FlagStore::FlagItem maybe_flag =
                store_.GetFlag(p.key);

            if (!maybe_flag) {
                return OffValue(flag,
                                EvaluationReason::PrerequisiteFailed(p.key));
            }

            flag_manager::FlagItemDescriptor const& descriptor = *maybe_flag;

            if (!descriptor.item) {
                // This flag existed at some point, but has since been deleted.
                return OffValue(flag,
                                EvaluationReason::PrerequisiteFailed(p.key));
            }

            // Recursive call; cycles are detected by the guard.
            EvaluationDetail<Value> detailed_evaluation =
                Evaluate(flag.key, *descriptor.item, context);

            if (detailed_evaluation.IsError()) {
                return detailed_evaluation;
            }

            std::optional<std::size_t> variation_index =
                detailed_evaluation.VariationIndex();

            // TODO(209589) prerequisite events.

            if (!descriptor.item->on || variation_index != p.variation) {
                return OffValue(flag,
                                EvaluationReason::PrerequisiteFailed(p.key));
            }
        }
    } else {
        LD_LOG(logger_, LogLevel::kError)
            << "Invalid flag configuration detected in flag \"" << parent_key
            << "\": prerequisite relationship to " << flag.key
            << " caused a circular reference; this is probably a temporary "
               "condition due to an incomplete update";
        return OffValue(flag, EvaluationReason(
                                  EvaluationReason::ErrorKind::kMalformedFlag));
    }

    // If the flag is on, all prerequisites are on and valid, then
    // determine if the context matches any targets.
    //
    // This happens before rule evaluation to ensure targets always have
    // priority.

    if (auto variation_index = AnyTargetMatchVariation(context, flag)) {
        return FlagVariation(flag, *variation_index,
                             EvaluationReason::TargetMatch());
    }

    for (std::size_t rule_index = 0; rule_index < flag.rules.size();
         rule_index++) {
        auto const& rule = flag.rules[rule_index];

        tl::expected<bool, Error> rule_match =
            Match(rule, context, store_, stack_);

        if (!rule_match) {
            LD_LOG(logger_, LogLevel::kError)
                << "Invalid flag configuration detected in flag \"" << flag.key
                << "\": " << rule_match.error();
            return EvaluationDetail<Value>(
                EvaluationReason::ErrorKind::kMalformedFlag, Value());
        }

        if (!(*rule_match)) {
            continue;
        }

        tl::expected<BucketResult, enum EvaluationReason::ErrorKind> result =
            ResolveVariationOrRollout(flag, rule.variationOrRollout, context);

        if (!result) {
            return EvaluationDetail<Value>(result.error(), Value());
        }

        EvaluationReason reason = EvaluationReason::RuleMatch(
            rule_index, rule.id, result->InExperiment());

        return FlagVariation(flag, result->VariationIndex(), std::move(reason));
    }

    // If there were no rule matches, then return the fallthrough variation.

    tl::expected<BucketResult, enum EvaluationReason::ErrorKind> result =
        ResolveVariationOrRollout(flag, flag.fallthrough, context);

    if (!result) {
        return EvaluationDetail<Value>(result.error(), Value());
    }

    EvaluationReason reason =
        EvaluationReason::Fallthrough(result->InExperiment());

    return FlagVariation(flag, result->VariationIndex(), std::move(reason));
}

EvaluationDetail<Value> Evaluator::FlagVariation(
    Flag const& flag,
    Flag::Variation variation_index,
    EvaluationReason reason) {
    if (variation_index >= flag.variations.size()) {
        LD_LOG(logger_, LogLevel::kError)
            << "Invalid flag configuration detected in flag \"" << flag.key
            << "\": rule, fallthrough, or target referenced a nonexistent "
               "variation index ("
            << variation_index << ")";
        return EvaluationDetail<Value>(
            EvaluationReason::ErrorKind::kMalformedFlag, Value());
    }

    return EvaluationDetail<Value>(flag.variations.at(variation_index),
                                   variation_index, std::move(reason));
}

EvaluationDetail<Value> Evaluator::OffValue(Flag const& flag,
                                            EvaluationReason reason) {
    if (flag.offVariation) {
        return FlagVariation(flag, *flag.offVariation, std::move(reason));
    }

    return EvaluationDetail<Value>(std::move(reason));
}

std::optional<std::size_t> AnyTargetMatchVariation(
    launchdarkly::Context const& context,
    Flag const& flag) {
    if (flag.contextTargets.empty()) {
        for (auto const& target : flag.targets) {
            if (auto index = TargetMatchVariation(context, target)) {
                return index;
            }
        }
        return std::nullopt;
    }

    for (auto const& context_target : flag.contextTargets) {
        if (context_target.contextKind == "user" &&
            context_target.values.empty()) {
            for (auto const& target : flag.targets) {
                if (target.variation == context_target.variation) {
                    if (auto index = TargetMatchVariation(context, target)) {
                        return index;
                    }
                }
            }
        } else if (auto index = TargetMatchVariation(context, context_target)) {
            return index;
        }
    }

    return std::nullopt;
}

std::optional<std::size_t> TargetMatchVariation(
    launchdarkly::Context const& context,
    Flag::Target const& target) {
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

tl::expected<BucketResult, Error> Variation(Flag::VariationOrRollout const& vr,
                                            std::string const& flag_key,
                                            Context const& context,
                                            std::string const& salt) {
    return std::visit(
        [&](auto&& arg) -> tl::expected<BucketResult, Error> {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, Flag::Variation>) {
                return BucketResult(arg);
            } else if constexpr (std::is_same_v<T, Flag::Rollout>) {
                if (arg.variations.empty()) {
                    return tl::make_unexpected(
                        Error::RolloutMissingVariations());
                }

                bool is_experiment =
                    arg.kind == Flag::Rollout::Kind::kExperiment;

                std::optional<BucketPrefix> prefix =
                    arg.seed ? BucketPrefix(*arg.seed)
                             : BucketPrefix(flag_key, salt);

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
                                lookup == RolloutKindLookup::kPresent);
                    }
                }

                return BucketResult(
                    arg.variations.back(),
                    is_experiment && lookup == RolloutKindLookup::kPresent);
            }
        },
        vr);
}

tl::expected<BucketResult, enum EvaluationReason::ErrorKind>
ResolveVariationOrRollout(Flag const& flag,
                          Flag::VariationOrRollout const& vr,
                          Context const& context) {
    if (!flag.salt) {
        return tl::make_unexpected(EvaluationReason::ErrorKind::kMalformedFlag);
    }

    return Variation(vr, flag.key, context, *flag.salt)
        .map_error([](Error discarded) {
            boost::ignore_unused(discarded);
            return EvaluationReason::ErrorKind::kMalformedFlag;
        });
}

}  // namespace launchdarkly::server_side::evaluation
