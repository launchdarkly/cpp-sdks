#include "bucketing.hpp"

#include <launchdarkly/detail/c_binding_helpers.hpp>
#include <launchdarkly/encoding/base_16.hpp>
#include <launchdarkly/encoding/sha_1.hpp>

#include <sstream>
#include <string_view>

namespace launchdarkly::server_side::evaluation {

using namespace launchdarkly::data_model;

double const kBucketHashScale = static_cast<double>(0x0FFFFFFFFFFFFFFF);

AttributeReference const& Key();

std::optional<ContextHashValue> ContextHash(Value const& value,
                                            BucketPrefix prefix);

std::optional<std::string> BucketValue(Value const& value);

bool IsIntegral(double f);

BucketPrefix::BucketPrefix(Seed seed) : prefix_(seed) {}

BucketPrefix::BucketPrefix(std::string key, std::string salt)
    : prefix_(KeyAndSalt{key, salt}) {}

std::ostream& operator<<(std::ostream& os, BucketPrefix const& prefix) {
    std::visit(
        [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, BucketPrefix::KeyAndSalt>) {
                os << arg.key << "." << arg.salt;
            } else if constexpr (std::is_same_v<T, BucketPrefix::Seed>) {
                os << arg;
            }
        },
        prefix.prefix_);
    return os;
}

BucketResult::BucketResult(Flag::Rollout::WeightedVariation weighted_variation,
                           bool is_experiment)
    : variation_index_(weighted_variation.variation),
      in_experiment_(is_experiment && !weighted_variation.untracked) {}

BucketResult::BucketResult(Flag::Variation variation, bool in_experiment)
    : variation_index_(variation), in_experiment_(in_experiment) {}

BucketResult::BucketResult(Flag::Variation variation)
    : variation_index_(variation), in_experiment_(false) {}

std::size_t BucketResult::VariationIndex() const {
    return variation_index_;
}

bool BucketResult::InExperiment() const {
    return in_experiment_;
}

tl::expected<std::pair<ContextHashValue, RolloutKindLookup>, Error> Bucket(
    Context const& context,
    AttributeReference const& by_attr,
    BucketPrefix const& prefix,
    bool is_experiment,
    std::string const& context_kind) {
    AttributeReference const& ref = is_experiment ? Key() : by_attr;

    if (!ref.Valid()) {
        return tl::make_unexpected(
            Error::InvalidAttributeReference(ref.RedactionName()));
    }

    Value value = context.Get(context_kind, ref);

    bool is_bucketable = value.Type() == Value::Type::kNumber ||
                         value.Type() == Value::Type::kString;

    if (is_bucketable) {
        return std::make_pair(ContextHash(value, prefix).value_or(0.0),
                              RolloutKindLookup::kPresent);
    }

    auto rollout_context_found =
        std::count(context.Kinds().begin(), context.Kinds().end(),
                   context_kind) > 0;

    return std::make_pair(0.0, rollout_context_found
                                   ? RolloutKindLookup::kPresent
                                   : RolloutKindLookup::kAbsent);
}

AttributeReference const& Key() {
    static AttributeReference const key{"key"};
    LD_ASSERT(key.Valid());
    return key;
}

std::optional<float> ContextHash(Value const& value, BucketPrefix prefix) {
    using namespace launchdarkly::encoding;

    std::optional<std::string> id = BucketValue(value);
    if (!id) {
        return std::nullopt;
    }

    std::stringstream input;
    input << prefix << "." << *id;

    std::array<unsigned char, SHA_DIGEST_LENGTH> const sha1hash =
        Sha1String(input.str());

    std::string const sha1hash_hexed = Base16Encode(sha1hash);

    std::string const sha1hash_hexed_first_15 = sha1hash_hexed.substr(0, 15);

    try {
        unsigned long long as_number =
            std::stoull(sha1hash_hexed_first_15.data(), nullptr, /* base */ 16);

        double as_double = static_cast<double>(as_number);
        return as_double / kBucketHashScale;

    } catch (std::invalid_argument) {
        return std::nullopt;
    } catch (std::out_of_range) {
        return std::nullopt;
    }
}

std::optional<std::string> BucketValue(Value const& value) {
    switch (value.Type()) {
        case Value::Type::kString:
            return value.AsString();
        case Value::Type::kNumber: {
            if (IsIntegral(value.AsDouble())) {
                return std::to_string(value.AsInt());
            }
            return std::nullopt;
        }
        default:
            return std::nullopt;
    }
}

bool IsIntegral(double f) {
    return std::trunc(f) == f;
}

tl::expected<BucketResult, Error> Variation(
    Flag::VariationOrRollout const& vr,
    std::string const& flag_key,
    Context const& context,
    std::optional<std::string> const& salt) {
    if (!salt) {
        return tl::make_unexpected(Error::MissingSalt(flag_key));
    }
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
                             : BucketPrefix(flag_key, *salt);

                auto bucketing_result = Bucket(context, arg.bucketBy, *prefix,
                                               is_experiment, arg.contextKind);
                if (!bucketing_result) {
                    return tl::make_unexpected(bucketing_result.error());
                }

                auto [bucket, lookup] = *bucketing_result;

                double sum = 0.0;

                for (const auto& variation : arg.variations) {
                    sum += variation.weight / kBucketScale;
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
}  // namespace launchdarkly::server_side::evaluation
