#pragma once

#include <launchdarkly/attribute_reference.hpp>
#include <launchdarkly/data_model/context_aware_reference.hpp>
#include <launchdarkly/serialization/json_context_kind.hpp>
#include <launchdarkly/detail/serialization/json_errors.hpp>
#include <launchdarkly/serialization/value_mapping.hpp>

#include <boost/json.hpp>
#include <tl/expected.hpp>

#include <optional>
#include <string>

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

    std::optional<data_model::ContextKind> kind;

    PARSE_CONDITIONAL_FIELD(kind, obj, Type::fields::kContextFieldName);

    std::string attr_ref_or_name;
    PARSE_FIELD_DEFAULT(attr_ref_or_name, obj,
                        Type::fields::kReferenceFieldName, "key");

    if (kind) {
        return Type{*kind,
                    AttributeReference::FromReferenceStr(attr_ref_or_name)};
    }

    return Type{data_model::ContextKind("user"),
                AttributeReference::FromLiteralStr(attr_ref_or_name)};
}

}  // namespace launchdarkly
