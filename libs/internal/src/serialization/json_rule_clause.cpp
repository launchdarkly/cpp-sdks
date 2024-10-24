#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>
#include <launchdarkly/serialization/json_context_aware_reference.hpp>
#include <launchdarkly/serialization/json_rule_clause.hpp>
#include <launchdarkly/detail/serialization/json_value.hpp>
#include <launchdarkly/serialization/value_mapping.hpp>

namespace launchdarkly {

tl::expected<std::optional<data_model::Clause>, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<std::optional<data_model::Clause>,
                                           JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    data_model::Clause clause{};

    PARSE_REQUIRED_FIELD(clause.op, obj, "op");
    PARSE_FIELD(clause.values, obj, "values");
    PARSE_FIELD(clause.negate, obj, "negate");

    auto kind_and_attr = boost::json::value_to<tl::expected<
        data_model::ContextAwareReference<data_model::Clause>, JsonError>>(
        json_value);
    if (!kind_and_attr) {
        return tl::make_unexpected(kind_and_attr.error());
    }

    clause.contextKind = kind_and_attr->contextKind;
    clause.attribute = kind_and_attr->reference;
    return clause;
}

tl::expected<std::optional<data_model::Clause::Op>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<data_model::Clause::Op>, JsonError>> const&
        unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_STRING(json_value);

    auto const& str = json_value.as_string();

    if (str == "") {
        // Treating empty string as indicating the field is absent, but could
        // also treat it as a valid but unknown value (like kUnrecognized.)
        return std::nullopt;
    } else if (str == "in") {
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
    return maybe_op.value().value_or(data_model::Clause::Op::kUnrecognized);
}

namespace data_model {

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                data_model::Clause const& clause) {
    auto& obj = json_value.emplace_object();

    obj.emplace("values", boost::json::value_from(clause.values));

    WriteMinimal(obj, "negate", clause.negate);

    if (clause.op != data_model::Clause::Op::kUnrecognized) {
        // TODO: Should we store the original value?
        obj.emplace("op", boost::json::value_from(clause.op));
    }
    if (clause.attribute.Valid()) {
        obj.emplace("attribute", clause.attribute.RedactionName());
    }
    obj.emplace("contextKind", clause.contextKind.t);
}

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                data_model::Clause::Op const& op) {
    switch (op) {
        case data_model::Clause::Op::kUnrecognized:
            // TODO: Should we do anything?
            break;
        case data_model::Clause::Op::kIn:
            json_value.emplace_string() = "in";
            break;
        case data_model::Clause::Op::kStartsWith:
            json_value.emplace_string() = "startsWith";
            break;
        case data_model::Clause::Op::kEndsWith:
            json_value.emplace_string() = "endsWith";
            break;
        case data_model::Clause::Op::kMatches:
            json_value.emplace_string() = "matches";
            break;
        case data_model::Clause::Op::kContains:
            json_value.emplace_string() = "contains";
            break;
        case data_model::Clause::Op::kLessThan:
            json_value.emplace_string() = "lessThan";
            break;
        case data_model::Clause::Op::kLessThanOrEqual:
            json_value.emplace_string() = "lessThanOrEqual";
            break;
        case data_model::Clause::Op::kGreaterThan:
            json_value.emplace_string() = "greaterThan";
            break;
        case data_model::Clause::Op::kGreaterThanOrEqual:
            json_value.emplace_string() = "greaterThanOrEqual";
            break;
        case data_model::Clause::Op::kBefore:
            json_value.emplace_string() = "before";
            break;
        case data_model::Clause::Op::kAfter:
            json_value.emplace_string() = "after";
            break;
        case data_model::Clause::Op::kSemVerEqual:
            json_value.emplace_string() = "semVerEqual";
            break;
        case data_model::Clause::Op::kSemVerLessThan:
            json_value.emplace_string() = "semVerLessThan";
            break;
        case data_model::Clause::Op::kSemVerGreaterThan:
            json_value.emplace_string() = "semVerGreaterThan";
            break;
        case data_model::Clause::Op::kSegmentMatch:
            json_value.emplace_string() = "segmentMatch";
            break;
    }
}
}  // namespace data_model

}  // namespace launchdarkly
