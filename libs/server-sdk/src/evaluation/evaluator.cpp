#include "evaluator.hpp"

#include <cmath>
#include <launchdarkly/attribute_reference.hpp>
#include <launchdarkly/data/evaluation_detail.hpp>
#include <launchdarkly/data/evaluation_reason.hpp>
#include <launchdarkly/detail/c_binding_helpers.hpp>
#include <launchdarkly/encoding/sha_1.hpp>
#include <launchdarkly/value.hpp>
#include <tl/expected.hpp>
#include <variant>
namespace launchdarkly::evaluation {

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
};

EvaluationDetail<Value> Variation(data_model::Flag const& flag,
                                  std::uint64_t variation_index,
                                  EvaluationReason reason);

EvaluationDetail<Value> OffValue(data_model::Flag const& flag,
                                 EvaluationReason reason);

tl::expected<std::optional<BucketResult>, enum EvaluationReason::ErrorKind>
Variation(data_model::Flag::VariationOrRollout const& vr,
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

void Bucket(Context const& context,
            AttributeReference const& by_attr,
            BucketPrefix const& prefix,
            bool is_experiment,
            std::string const& context_kind);

std::optional<float> ComputeBucket(Context const& context,
                                   Value const& value,
                                   BucketPrefix prefix,
                                   bool is_experiment);

std::optional<std::string> BucketValue(Value const& value);

Evaluator::Evaluator(Logger& logger) : logger_(logger), stack_(20) {}

EvaluationDetail<Value> Evaluator::Evaluate(data_model::Flag& flag,
                                            launchdarkly::Context& context) {
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
            std::optional<std::reference_wrapper<data_model::Flag>>
                prereq_flag = store_.flag(prereq.key);
            if (!prereq_flag) {
                return OffValue(
                    flag, EvaluationReason::PrerequisiteFailed(prereq.key));
            }
            if (stack_.SeenPrerequisite(prereq_flag->get().key)) {
                return EvaluationDetail<Value>(
                    EvaluationReason::ErrorKind::kMalformedFlag, Value());
            }
            EvaluationDetail<Value> prerequisite_result =
                Evaluate(prereq_flag, context);

            if (prerequisite_result.Reason().has_value() &&
                prerequisite_result.Reason().value().Kind() ==
                    EvaluationReason::Kind::kError) {
                return EvaluationDetail<Value>(
                    EvaluationReason::ErrorKind::kMalformedFlag, Value());
            }

            std::optional<std::size_t> variation_index =
                prerequisite_result.VariationIndex();

            // todo(cwaldren) record prerequisite evaluation

            if (!prereq_flag->get().on || !variation_index) {
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
        return Variation(flag, *variation_index,
                         EvaluationReason::TargetMatch());
    }

    // If there were no target matches, then evaluate against the
    // flag's rules.

    for (std::size_t i = 0; i < flag.rules.size(); i++) {
        auto const& rule = flag.rules[i];
        tl::expected<bool, enum EvaluationReason::ErrorKind> rule_match =
            Match(rule, context, store, stack_);
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
            EvaluationReason::RuleMatch(i, rule.id, result->inExperiment);
        return Variation(flag, rule_match->variationIndex, std::move(reason));
    }

    // If there were no rule matches, then return the fallthrough variation.

    auto result = ResolveVariationOrRollout(flag, flag.fallthrough, context);
    if (!result) {
        return EvaluationDetail<Value>(result.error(), Value());
    }
    auto reason = EvaluationReason::Fallthrough(result->inExperiment);
    return Variation(flag, result->variationIndex, std::move(reason));
}

EvaluationDetail<Value> Variation(data_model::Flag const& flag,
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
        return Variation(flag, *flag.offVariation, std::move(reason));
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
    auto has_kind = std::find(context.Kinds().begin(), context.Kinds().end(),
                              target.contextKind);
    if (has_kind == context.Kinds().end()) {
        return std::nullopt;
    }

    std::string const& key = context.Attributes(target.contextKind).Key();
    for (auto const& value : target.values) {
        if (value == key) {
            return target.variation;
        }
    }

    return std::nullopt;
}

tl::expected<std::optional<BucketResult>, enum EvaluationReason::ErrorKind>
Variation(data_model::Flag::VariationOrRollout const& vr,
          std::string const& flag_key,
          Context const& context,
          std::string const& salt) {
    return std::visit(
        [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, data_model::Flag::Variation>) {
                return BucketResult{arg, false};
            } else if constexpr (std::is_same_v<T, data_model::Flag::Rollout>) {
                bool is_experiment =
                    arg.kind == data_model::Flag::Rollout::Kind::kExperiment;

                std::optional<BucketPrefix> prefix;
                if (arg.seed) {
                    prefix = BucketPrefix(*arg.seed);
                } else {
                    prefix = BucketPrefix(flag_key, salt);
                }

                auto [bucket, bucket_and_was_missing_context] =
                    Bucket(context, arg.bucketBy, *prefix, is_experiment,
                           arg.contextKind);

                double sum = 0.0;

                for (const auto& variation : arg.variations) {
                    sum += variation.weight / 100000.0;
                    if (bucket < sum) {
                        return variation.as_bucket_result(
                            is_experiment && !bucket_and_was_missing_context)
                    }
                }

                return variations.last().as_bucket_result(
                    is_experiment && !bucket_and_was_missing_context);
            }
        },
        vr);
}
tl::expected<BucketResult, enum EvaluationReason::ErrorKind>
ResolveVariationOrRollout(data_model::Flag const& flag,
                          data_model::Flag::VariationOrRollout const& vr,
                          Context const& context) {
    auto const& result = Variation(vr, flag.key, context, flag.salt);
    if (!result) {
        return tl::make_unexpected(EvaluationReason::ErrorKind::kMalformedFlag);
    }
    auto const& bucket = result.value();
    if (!bucket) {
        return tl::make_unexpected(EvaluationReason::ErrorKind::kMalformedFlag);
    }
    return *bucket;
}

tl::expected<std::pair<float, bool /* context was missing */>, std::string>
Bucket(Context const& context,
       AttributeReference by_attr,
       BucketPrefix const& prefix,
       bool is_experiment,
       std::string const& context_kind) {
    AttributeReference ref =
        is_experiment ? AttributeReference("key") : std::move(by_attr);

    if (!ref.Valid()) {
        return tl::make_unexpected("invalid attribute reference");
    }

    Value value = context.Attributes(context_kind).Get(ref);

    if (!(value.Type() == Value::Type::kNumber ||
          value.Type() == Value::Type::kString)) {
        return std::make_pair(0.0, true);
    }
    return std::make_pair(
        ComputeBucket(value, prefix, is_experiment).value_or(0.0), false);
}

std::optional<float> ComputeBucket(Context const& context,
                                   Value const& value,
                                   BucketPrefix prefix,
                                   bool is_experiment) {
    LD_ASSERT(value.Type() == Value::Type::kNumber ||
              value.Type() == Value::Type::kString);

    std::optional<std::string> id = BucketValue(value);
    if (!id) {
        return std::nullopt;
    }

    std::string input;
    prefix.Update(&input);
    input.push_back('.');
    input.append(*id);

    auto hashed = launchdarkly::encoding::Sha1String(input);
}

inline bool IsIntegral(double f) {
    return std::trunc(f) == f;
}

std::optional<std::string> BucketValue(Value const& value) {
    switch (value.Type()) {
        case Value::Type::kString:
            return value.AsString();
        case Value::Type::kNumber: {
            if (IsIntegral(value.AsDouble())) {
                return std::to_string(value.AsInt());
            } else {
                return std::nullopt;
            }
        }
        default:
            return std::nullopt;
    }
}

}  // namespace launchdarkly::evaluation
