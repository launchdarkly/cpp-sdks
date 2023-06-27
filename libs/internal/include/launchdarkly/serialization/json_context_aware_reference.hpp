#pragma once

#include <boost/json.hpp>
#include <launchdarkly/attribute_reference.hpp>
#include <launchdarkly/data_model/context_aware_reference.hpp>
#include <launchdarkly/serialization/json_errors.hpp>
#include <launchdarkly/serialization/value_mapping.hpp>
#include <optional>
#include <string>
#include <tl/expected.hpp>

namespace launchdarkly {

template <typename Fields>
tl::expected<data_model::ContextAwareReference<Fields>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<data_model::ContextAwareReference<Fields>,
                     JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    using Type = data_model::ContextAwareReference<Fields>;

    auto const& obj = json_value.as_object();

    std::optional<std::string> kind;

    PARSE_CONDITIONAL_FIELD(kind, obj, Type::fields::kContextKindField);

    if (kind && *kind == "") {
        // Empty string is not a valid kind.
        return tl::make_unexpected(JsonError::kSchemaFailure);
    }

    std::string attr_ref_or_name;
    PARSE_FIELD_DEFAULT(attr_ref_or_name, obj, Type::fields::kReferenceField,
                        "key");

    if (kind) {
        return Type{*kind,
                    AttributeReference::FromReferenceStr(attr_ref_or_name)};
    }

    return Type{"user", AttributeReference::FromLiteralStr(attr_ref_or_name)};
}

}  // namespace launchdarkly
