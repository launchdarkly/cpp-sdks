#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>
#include <launchdarkly/serialization/json_primitives.hpp>
#include <launchdarkly/serialization/json_rule_clause.hpp>
#include <launchdarkly/serialization/json_segment.hpp>
#include <launchdarkly/serialization/value_mapping.hpp>

namespace launchdarkly {

tl::expected<std::optional<data_model::Segment::Target>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::Segment::Target>,
                     JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    data_model::Segment::Target target;

    PARSE_FIELD_DEFAULT(target.contextKind, obj, "contextKind", "user");

    PARSE_FIELD(target.values, obj, "values");

    return target;
}

tl::expected<ContextAwareReference<RolloutBucketing>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<ContextAwareReference<RolloutBucketing>, JsonError>> const&
        unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    auto const& obj = json_value.as_object();

    std::optional<std::string> kind;

    PARSE_CONDITIONAL_FIELD(
        kind, obj,
        ContextAwareReference<RolloutBucketing>::ContextKindFieldName());

    if (kind && *kind == "") {
        // Empty string is not a valid kind.
        return tl::make_unexpected(JsonError::kSchemaFailure);
    }

    std::string attr_ref_or_name;
    PARSE_FIELD_DEFAULT(
        attr_ref_or_name, obj,
        ContextAwareReference<RolloutBucketing>::ReferenceFieldName(), "key");

    if (kind) {
        return ContextAwareReference<RolloutBucketing>{
            *kind, AttributeReference::FromReferenceStr(attr_ref_or_name)};
    }

    return ContextAwareReference<RolloutBucketing>{
        "user", AttributeReference::FromLiteralStr(attr_ref_or_name)};
}

tl::expected<std::optional<data_model::Segment::Rule>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::Segment::Rule>,
                     JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    data_model::Segment::Rule rule;

    PARSE_FIELD(rule.clauses, obj, "clauses");

    PARSE_CONDITIONAL_FIELD(rule.weight, obj, "weight");
    PARSE_CONDITIONAL_FIELD(rule.id, obj, "id");

    auto kind_and_bucket_by = boost::json::value_to<
        tl::expected<ContextAwareReference<RolloutBucketing>, JsonError>>(
        json_value);
    if (!kind_and_bucket_by) {
        return tl::make_unexpected(kind_and_bucket_by.error());
    }

    rule.rolloutContextKind = kind_and_bucket_by->contextKind;
    rule.bucketBy = kind_and_bucket_by->reference;

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

    PARSE_CONDITIONAL_FIELD(segment.generation, obj, "generation");
    PARSE_CONDITIONAL_FIELD(segment.salt, obj, "salt");
    PARSE_CONDITIONAL_FIELD(segment.unboundedContextKind, obj,
                            "unboundedContextKind");

    PARSE_FIELD(segment.excluded, obj, "excluded");
    PARSE_FIELD(segment.included, obj, "included");
    PARSE_FIELD(segment.unbounded, obj, "unbounded");
    PARSE_FIELD(segment.includedContexts, obj, "includedContexts");
    PARSE_FIELD(segment.excludedContexts, obj, "excludedContexts");
    PARSE_FIELD(segment.rules, obj, "rules");

    return segment;
}

}  // namespace launchdarkly
