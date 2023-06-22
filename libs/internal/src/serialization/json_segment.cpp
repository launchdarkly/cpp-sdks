#include <boost/core/ignore_unused.hpp>
#include <launchdarkly/serialization/json_primitives.hpp>
#include <launchdarkly/serialization/json_rule_clause.hpp>
#include <launchdarkly/serialization/json_segment.hpp>
#include <launchdarkly/serialization/value_mapping.hpp>

namespace launchdarkly {

tl::expected<data_model::Segment::Target, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<data_model::Segment::Target, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    data_model::Segment::Target target;

    PARSE_REQUIRED_FIELD(target.contextKind, obj, "contextKind");
    PARSE_REQUIRED_FIELD(target.values, obj, "values");

    return target;
}

tl::expected<data_model::Segment::Rule, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<data_model::Segment::Rule, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    data_model::Segment::Rule rule;

    PARSE_REQUIRED_FIELD(rule.clauses, obj, "clauses");

    PARSE_OPTIONAL_FIELD(rule.rolloutContextKind, obj, "rolloutContextKind");
    PARSE_OPTIONAL_FIELD(rule.weight, obj, "weight");
    PARSE_OPTIONAL_FIELD(rule.id, obj, "id");

    std::optional<std::string> literal_or_ref;
    PARSE_OPTIONAL_FIELD(literal_or_ref, obj, "bucketBy");

    rule.bucketBy = MapOpt<AttributeReference, std::string>(
        literal_or_ref,
        [has_context = rule.rolloutContextKind.has_value()](auto&& ref) {
            if (has_context) {
                return AttributeReference::FromReferenceStr(ref);
            } else {
                return AttributeReference::FromLiteralStr(ref);
            }
        });

    return rule;
}

tl::expected<std::optional<data_model::Segment>, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<std::optional<data_model::Segment>,
                                           JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (json_value.is_null()) {
        return std::nullopt;
    }

    if (!json_value.is_object()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }

    if (json_value.as_object().empty()) {
        return std::nullopt;
    }

    auto const& obj = json_value.as_object();

    data_model::Segment segment;

    PARSE_REQUIRED_FIELD(segment.key, obj, "key");
    PARSE_REQUIRED_FIELD(segment.version, obj, "version");

    PARSE_OPTIONAL_FIELD(segment.excluded, obj, "excluded");
    PARSE_OPTIONAL_FIELD(segment.included, obj, "included");

    PARSE_OPTIONAL_FIELD(segment.generation, obj, "generation");
    PARSE_OPTIONAL_FIELD(segment.salt, obj, "salt");
    PARSE_OPTIONAL_FIELD(segment.unbounded, obj, "unbounded");

    PARSE_OPTIONAL_FIELD(segment.includedContexts, obj, "includedContexts");
    PARSE_OPTIONAL_FIELD(segment.excludedContexts, obj, "excludedContexts");

    PARSE_OPTIONAL_FIELD(segment.rules, obj, "rules");

    return segment;
}

}  // namespace launchdarkly
