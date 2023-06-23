#include <boost/json.hpp>
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
    PARSE_REQUIRED_FIELD(rollout.variations, obj, "variations");
    PARSE_OPTIONAL_FIELD(rollout.contextKind, obj, "contextKind");
    PARSE_OPTIONAL_FIELD(rollout.kind, obj, "kind");
    PARSE_OPTIONAL_FIELD(rollout.seed, obj, "seed");

    std::optional<std::string> literal_or_ref;
    PARSE_OPTIONAL_FIELD(literal_or_ref, obj, "bucketBy");

    rollout.bucketBy = MapOpt<AttributeReference, std::string>(
        literal_or_ref,
        [has_context = rollout.contextKind.has_value()](auto&& ref) {
            if (has_context) {
                return AttributeReference::FromReferenceStr(ref);
            } else {
                return AttributeReference::FromLiteralStr(ref);
            }
        });

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
    PARSE_REQUIRED_FIELD(weighted_variation.variation, obj, "variation");
    PARSE_REQUIRED_FIELD(weighted_variation.weight, obj, "weight");
    PARSE_OPTIONAL_FIELD(weighted_variation.untracked, obj, "untracked");
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
    PARSE_REQUIRED_FIELD(prerequisite.variation, obj, "variation");
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
    PARSE_REQUIRED_FIELD(target.values, obj, "values");
    PARSE_REQUIRED_FIELD(target.variation, obj, "variation");
    PARSE_OPTIONAL_FIELD(target.contextKind, obj, "contextKind");
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
    PARSE_OPTIONAL_FIELD(rule.id, obj, "id");
    PARSE_OPTIONAL_FIELD(rule.trackEvents, obj, "trackEvents");
    PARSE_REQUIRED_FIELD(rule.clauses, obj, "clauses");

    auto variation_or_rollout = boost::json::value_to<tl::expected<
        std::optional<data_model::Flag::VariationOrRollout>, JsonError>>(
        json_value);
    if (!variation_or_rollout) {
        return tl::make_unexpected(variation_or_rollout.error());
    }

    rule.variation_or_rollout =
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
    PARSE_OPTIONAL_FIELD(client_side_availability.usingEnvironmentId, obj,
                         "usingEnvironmentId");
    PARSE_OPTIONAL_FIELD(client_side_availability.usingMobileKey, obj,
                         "usingMobileKey");
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
    PARSE_REQUIRED_FIELD(flag.version, obj, "version");
    PARSE_REQUIRED_FIELD(flag.on, obj, "on");
    PARSE_REQUIRED_FIELD(flag.variations, obj, "variations");

    PARSE_OPTIONAL_FIELD(flag.prerequisites, obj, "prerequisites");
    PARSE_OPTIONAL_FIELD(flag.targets, obj, "targets");
    PARSE_OPTIONAL_FIELD(flag.contextTargets, obj, "contextTargets");
    PARSE_OPTIONAL_FIELD(flag.rules, obj, "rules");
    PARSE_OPTIONAL_FIELD(flag.offVariation, obj, "offVariation");
    PARSE_OPTIONAL_FIELD(flag.clientSide, obj, "clientSide");
    PARSE_OPTIONAL_FIELD(flag.clientSideAvailability, obj,
                         "clientSideAvailability");
    PARSE_OPTIONAL_FIELD(flag.salt, obj, "salt");
    PARSE_OPTIONAL_FIELD(flag.trackEvents, obj, "trackEvents");
    PARSE_OPTIONAL_FIELD(flag.trackEventsFallthrough, obj,
                         "trackEventsFallthrough");
    PARSE_OPTIONAL_FIELD(flag.debugEventsUntilDate, obj,
                         "debugEventsUntilDate");

    auto variation_or_rollout = boost::json::value_to<tl::expected<
        std::optional<data_model::Flag::VariationOrRollout>, JsonError>>(
        json_value);
    if (!variation_or_rollout) {
        return tl::make_unexpected(variation_or_rollout.error());
    }

    flag.fallthrough =
        variation_or_rollout->value_or(data_model::Flag::Variation(0));

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
    PARSE_OPTIONAL_FIELD(rollout, obj, "rollout");

    if (rollout) {
        return std::make_optional(*rollout);
    }

    data_model::Flag::Variation variation;
    PARSE_REQUIRED_FIELD(variation, obj, "variation");

    return std::make_optional(variation);
}

}  // namespace launchdarkly
