#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>
#include <launchdarkly/serialization/json_rule_clause.hpp>
#include <launchdarkly/serialization/json_value.hpp>
#include <launchdarkly/serialization/value_mapping.hpp>

namespace launchdarkly {

tl::expected<data_model::Clause, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<data_model::Clause, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    data_model::Clause clause;

    PARSE_REQUIRED_FIELD(clause.op, obj, "op");
    PARSE_REQUIRED_FIELD(clause.values, obj, "values");

    PARSE_OPTIONAL_FIELD(clause.negate, obj, "negate");
    PARSE_OPTIONAL_FIELD(clause.contextKind, obj, "contextKind");

    std::optional<std::string> literal_or_ref;
    PARSE_OPTIONAL_FIELD(literal_or_ref, obj, "attribute");

    clause.attribute = MapOpt<AttributeReference, std::string>(
        literal_or_ref,
        [has_context = clause.contextKind.has_value()](auto&& ref) {
            if (has_context) {
                return AttributeReference::FromReferenceStr(ref);
            } else {
                return AttributeReference::FromLiteralStr(ref);
            }
        });

    return clause;
}

tl::expected<std::optional<data_model::Clause::Op>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::Clause::Op>, JsonError>> const&
        unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    if (json_value.is_null()) {
        return std::nullopt;
    }
    if (!json_value.is_string()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    if (json_value.as_string().empty()) {
        return std::nullopt;
    }
    auto const& str = json_value.as_string();

    if (str == "in") {
        return data_model::Clause::Op::kIn;
    } else if (str == "endsWith") {
        return data_model::Clause::Op::kEndsWith;
    } else if (str == "startsWith") {
        return data_model::Clause::Op::kStartsWith;
    } else if (str == "matches") {
        return data_model::Clause::Op::kMatches;
    } else if (str == "contains") {
        return data_model::Clause::Op::kContains;
    } else if (str == "lessThan") {
        return data_model::Clause::Op::kLessThan;
    } else if (str == "lessThanOrEqual") {
        return data_model::Clause::Op::kLessThanOrEqual;
    } else if (str == "greaterThan") {
        return data_model::Clause::Op::kGreaterThan;
    } else if (str == "greaterThanOrEqual") {
        return data_model::Clause::Op::kGreaterThanOrEqual;
    } else if (str == "before") {
        return data_model::Clause::Op::kBefore;
    } else if (str == "after") {
        return data_model::Clause::Op::kAfter;
    } else if (str == "semVerEqual") {
        return data_model::Clause::Op::kSemVerEqual;
    } else if (str == "semVerLessThan") {
        return data_model::Clause::Op::kSemVerLessThan;
    } else if (str == "semVerGreaterThan") {
        return data_model::Clause::Op::kSemVerGreaterThan;
    } else if (str == "segmentMatch") {
        return data_model::Clause::Op::kSegmentMatch;
    } else {
        return data_model::Clause::Op::kUnrecognized;
    }
}

tl::expected<data_model::Clause::Op, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<data_model::Clause::Op, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    auto maybe_op = boost::json::value_to<
        tl::expected<std::optional<data_model::Clause::Op>, JsonError>>(
        json_value);
    if (!maybe_op) {
        return tl::unexpected(maybe_op.error());
    }
    return maybe_op.value().value_or(data_model::Clause::Op::kUnspecified);
}

}  // namespace launchdarkly
