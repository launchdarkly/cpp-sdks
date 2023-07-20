#include <boost/json.hpp>
#include <launchdarkly/serialization/json_context_aware_reference.hpp>
#include <launchdarkly/serialization/json_context_kind.hpp>
#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_primitives.hpp>
#include <launchdarkly/serialization/json_rule_clause.hpp>
#include <launchdarkly/serialization/json_value.hpp>
#include <launchdarkly/serialization/value_mapping.hpp>

namespace launchdarkly {

tl::expected<std::optional<data_model::Flag::Rollout>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::Flag::Rollout>,
                     JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    data_model::Flag::Rollout rollout;

    PARSE_FIELD(rollout.variations, obj, "variations");
    PARSE_FIELD_DEFAULT(rollout.kind, obj, "kind",
                        data_model::Flag::Rollout::Kind::kRollout);
    PARSE_CONDITIONAL_FIELD(rollout.seed, obj, "seed");

    auto kind_and_bucket_by = boost::json::value_to<tl::expected<
        data_model::ContextAwareReference<data_model::Flag::Rollout>,
        JsonError>>(json_value);
    if (!kind_and_bucket_by) {
        return tl::make_unexpected(kind_and_bucket_by.error());
    }

    rollout.contextKind = kind_and_bucket_by->contextKind;
    rollout.bucketBy = kind_and_bucket_by->reference;

    return rollout;
}

tl::expected<std::optional<data_model::Flag::Rollout::WeightedVariation>,
             JsonError>
tag_invoke(boost::json::value_to_tag<tl::expected<
               std::optional<data_model::Flag::Rollout::WeightedVariation>,
               JsonError>> const& unused,
           boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    data_model::Flag::Rollout::WeightedVariation weighted_variation;
    PARSE_FIELD(weighted_variation.variation, obj, "variation");
    PARSE_FIELD(weighted_variation.weight, obj, "weight");
    PARSE_FIELD(weighted_variation.untracked, obj, "untracked");
    return weighted_variation;
}

tl::expected<std::optional<data_model::Flag::Rollout::Kind>, JsonError>
tag_invoke(boost::json::value_to_tag<
               tl::expected<std::optional<data_model::Flag::Rollout::Kind>,
                            JsonError>> const& unused,
           boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    REQUIRE_STRING(json_value);

    auto const& str = json_value.as_string();
    if (str == "experiment") {
        return data_model::Flag::Rollout::Kind::kExperiment;
    } else if (str == "rollout") {
        return data_model::Flag::Rollout::Kind::kRollout;
    } else {
        return data_model::Flag::Rollout::Kind::kUnrecognized;
    }
}

tl::expected<std::optional<data_model::Flag::Prerequisite>, JsonError>
tag_invoke(boost::json::value_to_tag<
               tl::expected<std::optional<data_model::Flag::Prerequisite>,
                            JsonError>> const& unused,
           boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    data_model::Flag::Prerequisite prerequisite;
    PARSE_REQUIRED_FIELD(prerequisite.key, obj, "key");
    PARSE_FIELD(prerequisite.variation, obj, "variation");
    return prerequisite;
}

tl::expected<std::optional<data_model::Flag::Target>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::Flag::Target>, JsonError>> const&
        unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    data_model::Flag::Target target;
    PARSE_FIELD(target.values, obj, "values");
    PARSE_FIELD(target.variation, obj, "variation");
    PARSE_FIELD_DEFAULT(target.contextKind, obj, "contextKind",
                        data_model::ContextKind("user"));
    return target;
}

tl::expected<std::optional<data_model::Flag::Rule>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::Flag::Rule>, JsonError>> const&
        unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    data_model::Flag::Rule rule;

    PARSE_FIELD(rule.trackEvents, obj, "trackEvents");
    PARSE_FIELD(rule.clauses, obj, "clauses");
    PARSE_CONDITIONAL_FIELD(rule.id, obj, "id");

    auto variation_or_rollout = boost::json::value_to<tl::expected<
        std::optional<data_model::Flag::VariationOrRollout>, JsonError>>(
        json_value);
    if (!variation_or_rollout) {
        return tl::make_unexpected(variation_or_rollout.error());
    }

    rule.variationOrRollout =
        variation_or_rollout->value_or(data_model::Flag::Variation(0));

    return rule;
}

tl::expected<std::optional<data_model::Flag::ClientSideAvailability>, JsonError>
tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::Flag::ClientSideAvailability>,
                     JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    data_model::Flag::ClientSideAvailability client_side_availability;
    PARSE_FIELD(client_side_availability.usingEnvironmentId, obj,
                "usingEnvironmentId");
    PARSE_FIELD(client_side_availability.usingMobileKey, obj, "usingMobileKey");
    return client_side_availability;
}

tl::expected<std::optional<data_model::Flag>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::Flag>, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);

    auto const& obj = json_value.as_object();

    data_model::Flag flag;

    PARSE_REQUIRED_FIELD(flag.key, obj, "key");

    PARSE_CONDITIONAL_FIELD(flag.debugEventsUntilDate, obj,
                            "debugEventsUntilDate");
    PARSE_CONDITIONAL_FIELD(flag.salt, obj, "salt");
    PARSE_CONDITIONAL_FIELD(flag.offVariation, obj, "offVariation");

    PARSE_FIELD(flag.version, obj, "version");
    PARSE_FIELD(flag.on, obj, "on");
    PARSE_FIELD(flag.variations, obj, "variations");

    PARSE_FIELD(flag.prerequisites, obj, "prerequisites");
    PARSE_FIELD(flag.targets, obj, "targets");
    PARSE_FIELD(flag.contextTargets, obj, "contextTargets");
    PARSE_FIELD(flag.rules, obj, "rules");
    PARSE_FIELD(flag.clientSide, obj, "clientSide");
    PARSE_FIELD(flag.clientSideAvailability, obj, "clientSideAvailability");
    PARSE_FIELD(flag.trackEvents, obj, "trackEvents");
    PARSE_FIELD(flag.trackEventsFallthrough, obj, "trackEventsFallthrough");
    PARSE_FIELD(flag.fallthrough, obj, "fallthrough");
    return flag;
}

tl::expected<std::optional<data_model::Flag::VariationOrRollout>, JsonError>
tag_invoke(boost::json::value_to_tag<
               tl::expected<std::optional<data_model::Flag::VariationOrRollout>,
                            JsonError>> const& unused,
           boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    std::optional<data_model::Flag::Rollout> rollout;
    PARSE_CONDITIONAL_FIELD(rollout, obj, "rollout");

    if (rollout) {
        return std::make_optional(*rollout);
    }

    data_model::Flag::Variation variation;
    PARSE_REQUIRED_FIELD(variation, obj, "variation");

    return std::make_optional(variation);
}

namespace data_model {
void tag_invoke(
    boost::json::value_from_tag const& unused,
    boost::json::value& json_value,
    data_model::Flag::Rollout::WeightedVariation const& weighted_variation) {
    auto& obj = json_value.emplace_object();
    obj.emplace("variation", weighted_variation.variation);
    obj.emplace("weight", weighted_variation.weight);

    WriteMinimal(obj, "untracked", weighted_variation.untracked);
}

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                data_model::Flag::Rollout::Kind const& kind) {
    switch (kind) {
        case Flag::Rollout::Kind::kUnrecognized:
            // TODO: Should we be preserving the original string.
            break;
        case Flag::Rollout::Kind::kExperiment:
            json_value.emplace_string() = "experiment";
            break;
        case Flag::Rollout::Kind::kRollout:
            json_value.emplace_string() = "rollout";
            break;
    }
}

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                data_model::Flag::Rollout const& rollout) {
    auto& obj = json_value.emplace_object();

    obj.emplace("variations", boost::json::value_from(rollout.variations));
    if (rollout.kind != Flag::Rollout::Kind::kUnrecognized) {
        // TODO: Should we be preserving the original string and putting it in.
        obj.emplace("kind", boost::json::value_from(rollout.kind));
    }
    WriteMinimal(obj, "seed", rollout.seed);
    if (rollout.bucketBy.Valid()) {
        obj.emplace("bucketBy", rollout.bucketBy.RedactionName());
    }
    obj.emplace("contextKind", rollout.contextKind.t);
}

void tag_invoke(
    boost::json::value_from_tag const& unused,
    boost::json::value& json_value,
    data_model::Flag::VariationOrRollout const& variation_or_rollout) {
    auto& obj = json_value.emplace_object();
    std::visit(
        [&obj](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, data_model::Flag::Rollout>) {
                obj.emplace("rollout", boost::json::value_from(arg));
            } else if constexpr (std::is_same_v<T,
                                                data_model::Flag::Variation>) {
                obj.emplace("variation", arg);
            }
        },
        variation_or_rollout);
}

}  // namespace data_model

}  // namespace launchdarkly
