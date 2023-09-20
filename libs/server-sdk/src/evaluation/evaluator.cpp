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

Evaluator::Evaluator(Logger& logger, data_sources::IDataSource const& source)
    : logger_(logger), source_(source), stack_() {}

EvaluationDetail<Value> Evaluator::Evaluate(
    data_model::Flag const& flag,
    launchdarkly::Context const& context) {
    return Evaluate(flag, context, EventScope{});
}

EvaluationDetail<Value> Evaluator::Evaluate(
    Flag const& flag,
    launchdarkly::Context const& context,
    EventScope const& event_scope) {
    return Evaluate(std::nullopt, flag, context, event_scope);
}

EvaluationDetail<Value> Evaluator::Evaluate(
    std::optional<std::string> parent_key,
    Flag const& flag,
    launchdarkly::Context const& context,
    EventScope const& event_scope) {
    if (auto guard = stack_.NoticePrerequisite(flag.key)) {
        if (!flag.on) {
            return OffValue(flag, EvaluationReason::Off());
        }

        for (Flag::Prerequisite const& p : flag.prerequisites) {
            std::shared_ptr<data_sources::FlagDescriptor> maybe_flag =
                source_.GetFlag(p.key);

            if (!maybe_flag) {
                return OffValue(flag,
                                EvaluationReason::PrerequisiteFailed(p.key));
            }

            data_sources::FlagDescriptor const& descriptor = *maybe_flag;

            if (!descriptor.item) {
                // This flag existed at some point, but has since been deleted.
                return OffValue(flag,
                                EvaluationReason::PrerequisiteFailed(p.key));
            }

            // Recursive call; cycles are detected by the guard.
            EvaluationDetail<Value> detailed_evaluation =
                Evaluate(flag.key, *descriptor.item, context, event_scope);

            if (detailed_evaluation.IsError()) {
                return detailed_evaluation;
            }

            std::optional<std::size_t> variation_index =
                detailed_evaluation.VariationIndex();

            event_scope.Send([&](EventFactory const& factory) {
                return factory.Eval(p.key, context, *descriptor.item,
                                    detailed_evaluation, Value::Null(),
                                    flag.key);
            });

            if (!descriptor.item->on || variation_index != p.variation) {
                return OffValue(flag,
                                EvaluationReason::PrerequisiteFailed(p.key));
            }
        }
    } else {
        LogError(parent_key.value_or("(no parent)"),
                 Error::CyclicPrerequisiteReference(flag.key));
        return EvaluationReason::MalformedFlag();
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
            Match(rule, context, source_, stack_);

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
    if (variation_index < 0 || variation_index >= flag.variations.size()) {
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
