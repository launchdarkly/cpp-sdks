#include "evaluator.hpp"
#include "rules.hpp"

#include <boost/core/ignore_unused.hpp>
#include <tl/expected.hpp>

#include <algorithm>
#include <variant>

namespace launchdarkly::server_side::evaluation {

using namespace data_model;

std::optional<std::size_t> AnyTargetMatchVariation(
    launchdarkly::Context const& context,
    Flag const& flag);

std::optional<std::size_t> TargetMatchVariation(
    launchdarkly::Context const& context,
    Flag::Target const& target);

Evaluator::Evaluator(Logger& logger, data_store::IDataStore const& store)
    : logger_(logger), store_(store), stack_() {}

EvaluationDetail<Value> Evaluator::Evaluate(
    Flag const& flag,
    launchdarkly::Context const& context) const {
    return Evaluate("", flag, context);
}

EvaluationDetail<Value> Evaluator::Evaluate(
    std::string const& parent_key,
    Flag const& flag,
    launchdarkly::Context const& context) const {
    if (auto guard = stack_.NoticePrerequisite(flag.key)) {
        if (!flag.on) {
            return OffValue(flag, EvaluationReason::Off());
        }

        for (Flag::Prerequisite const& p : flag.prerequisites) {
            std::shared_ptr<data_store::FlagDescriptor> maybe_flag =
                store_.GetFlag(p.key);

            if (!maybe_flag) {
                return OffValue(flag,
                                EvaluationReason::PrerequisiteFailed(p.key));
            }

            data_store::FlagDescriptor const& descriptor = *maybe_flag;

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
        LogError(parent_key, Error::CyclicPrerequisiteReference(flag.key));
        return OffValue(flag, EvaluationReason::MalformedFlag());
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
            LogError(flag.key, rule_match.error());
            return EvaluationReason::MalformedFlag();
        }

        if (!(rule_match.value())) {
            continue;
        }

        tl::expected<BucketResult, Error> result =
            Variation(rule.variationOrRollout, flag.key, context, flag.salt);

        if (!result) {
            LogError(flag.key, result.error());
            return EvaluationReason::MalformedFlag();
        }

        EvaluationReason reason = EvaluationReason::RuleMatch(
            rule_index, rule.id, result->InExperiment());

        return FlagVariation(flag, result->VariationIndex(), std::move(reason));
    }

    // If there were no rule matches, then return the fallthrough variation.

    tl::expected<BucketResult, Error> result =
        Variation(flag.fallthrough, flag.key, context, flag.salt);

    if (!result) {
        LogError(flag.key, result.error());
        return EvaluationReason::MalformedFlag();
    }

    EvaluationReason reason =
        EvaluationReason::Fallthrough(result->InExperiment());

    return FlagVariation(flag, result->VariationIndex(), std::move(reason));
}

EvaluationDetail<Value> Evaluator::FlagVariation(
    Flag const& flag,
    Flag::Variation variation_index,
    EvaluationReason reason) const {
    if (variation_index >= flag.variations.size()) {
        LogError(flag.key, Error::NonexistentVariationIndex(variation_index));
        return EvaluationReason::MalformedFlag();
    }

    return {flag.variations.at(variation_index), variation_index,
            std::move(reason)};
}

EvaluationDetail<Value> Evaluator::OffValue(Flag const& flag,
                                            EvaluationReason reason) const {
    if (flag.offVariation) {
        return FlagVariation(flag, *flag.offVariation, std::move(reason));
    }

    return reason;
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
        if (IsUser(context_target.contextKind) &&
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
    Value const& key = context.Get(target.contextKind, "key");
    if (key.IsNull()) {
        return std::nullopt;
    }

    for (auto const& value : target.values) {
        if (value == key) {
            return target.variation;
        }
    }

    return std::nullopt;
}

void Evaluator::LogError(std::string const& key, Error const& error) const {
    LD_LOG(logger_, LogLevel::kError)
        << "Invalid flag configuration detected in flag \"" << key
        << "\": " << error;
}

}  // namespace launchdarkly::server_side::evaluation
