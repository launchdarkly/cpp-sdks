#include "bucketing.hpp"
#include <launchdarkly/detail/c_binding_helpers.hpp>
#include <launchdarkly/encoding/base_16.hpp>
#include <launchdarkly/encoding/sha_1.hpp>
#include <string_view>

namespace launchdarkly::server_side::evaluation {

constexpr std::int64_t kBucketScaleInt = 0x0FFFFFFFFFFFFFFF;

static_assert(kBucketScaleInt < std::numeric_limits<double>::max(),
              "Bucket scale is too large to be represented as a double");
constexpr double kBucketScaleDouble = static_cast<double>(kBucketScaleInt);

tl::expected<std::pair<ContextHashValue, RolloutKindPresence>, char const*>
Bucket(Context const& context,
       AttributeReference const& by_attr,
       BucketPrefix const& prefix,
       bool is_experiment,
       std::string const& context_kind) {
    AttributeReference ref =
        is_experiment ? AttributeReference("key") : std::move(by_attr);

    if (!ref.Valid()) {
        return tl::make_unexpected("invalid attribute reference");
    }

    Value value = context.Get(context_kind, ref);

    bool is_bucketable = value.Type() == Value::Type::kNumber ||
                         value.Type() == Value::Type::kString;

    if (is_bucketable) {
        return std::make_pair(
            ComputeBucket(value, prefix, is_experiment).value_or(0.0),
            RolloutKindPresence::kPresent);
    }

    auto rollout_context_found =
        std::count(context.Kinds().begin(), context.Kinds().end(),
                   context_kind) > 0
            ? RolloutKindPresence::kPresent
            : RolloutKindPresence::kAbsent;

    return std::make_pair(0.0, rollout_context_found);
}

std::optional<float> ComputeBucket(Value const& value,
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

    auto sha1hash = launchdarkly::encoding::Sha1String(input);
    auto sha1hash_hexed = launchdarkly::encoding::Base16Encode(sha1hash);
    std::string_view sha1hash_hexed_first_15 = {sha1hash_hexed.data(), 15};

    try {
        unsigned long long as_number =
            std::stoull(sha1hash_hexed_first_15.data(), nullptr, 16);

        double as_double = static_cast<double>(as_number);
        return as_double / kBucketScaleDouble;

    } catch (std::invalid_argument) {
        return std::nullopt;
    } catch (std::out_of_range) {
        return std::nullopt;
    }
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

}  // namespace launchdarkly::server_side::evaluation
