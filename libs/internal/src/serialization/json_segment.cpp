#include <boost/core/ignore_unused.hpp>
#include <launchdarkly/serialization/json_primitives.hpp>
#include <launchdarkly/serialization/json_segment.hpp>
#include <launchdarkly/serialization/json_value.hpp>
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

tl::expected<data_model::Segment::Clause, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<data_model::Segment::Clause, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    data_model::Segment::Clause clause;

    PARSE_OPTIONAL_FIELD(clause.contextKind, obj, "contextKind");

    std::string literal_or_ref;

    PARSE_REQUIRED_FIELD(literal_or_ref, obj, "attribute");

    if (clause.contextKind) {
        clause.attribute =
            AttributeReference::FromReferenceStr(std::move(literal_or_ref));
    } else {
        clause.attribute =
            AttributeReference::FromLiteralStr(std::move(literal_or_ref));
    }

    PARSE_REQUIRED_FIELD(clause.op, obj, "op");
    PARSE_REQUIRED_FIELD(clause.values, obj, "values");
    PARSE_OPTIONAL_FIELD(clause.negate, obj, "negate");

    return clause;
}

tl::expected<data_model::Segment::Clause::Op, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<data_model::Segment::Clause::Op, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_STRING(json_value);

    auto const& str = json_value.as_string();

    if (str == "in") {
        return data_model::Segment::Clause::Op::kIn;
    } else if (str == "endsWith") {
        return data_model::Segment::Clause::Op::kEndsWith;
    } else if (str == "startsWith") {
        return data_model::Segment::Clause::Op::kStartsWith;
    } else if (str == "matches") {
        return data_model::Segment::Clause::Op::kMatches;
    } else if (str == "contains") {
        return data_model::Segment::Clause::Op::kContains;
    } else if (str == "lessThan") {
        return data_model::Segment::Clause::Op::kLessThan;
    } else if (str == "lessThanOrEqual") {
        return data_model::Segment::Clause::Op::kLessThanOrEqual;
    } else if (str == "greaterThan") {
        return data_model::Segment::Clause::Op::kGreaterThan;
    } else if (str == "greaterThanOrEqual") {
        return data_model::Segment::Clause::Op::kGreaterThanOrEqual;
    } else if (str == "before") {
        return data_model::Segment::Clause::Op::kBefore;
    } else if (str == "after") {
        return data_model::Segment::Clause::Op::kAfter;
    } else if (str == "semVerEqual") {
        return data_model::Segment::Clause::Op::kSemVerEqual;
    } else if (str == "semVerLessThan") {
        return data_model::Segment::Clause::Op::kSemVerLessThan;
    } else if (str == "semVerGreaterThan") {
        return data_model::Segment::Clause::Op::kSemVerGreaterThan;
    } else if (str == "segmentMatch") {
        return data_model::Segment::Clause::Op::kSegmentMatch;
    } else {
        return data_model::Segment::Clause::Op::kUnrecognized;
    }
}

tl::expected<data_model::Segment, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<data_model::Segment, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);

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
