#pragma once

#include <launchdarkly/attribute_reference.hpp>
#include <launchdarkly/data_model/context_kind.hpp>

#include <string>

namespace launchdarkly::data_model {

/**
 * The JSON data conditionally contains Attribute References (which are capable
 * of addressing arbitrarily nested attributes in contexts) or Attribute Names,
 * which are names of top-level attributes in contexts.
 *
 * In order to distinguish these two cases, inspection of a context kind field
 * is necessary. The presence or absence of that field determines whether the
 * data is an Attribute Reference or Attribute Name.
 *
 * Because this logic is needed in (3) places, it is factored out into this
 * type. To use it, call
 * boost::json::value_from<tl::expected<ContextAwareReference<T>,
 * JsonError>>(json_value), where T is any type that defines the following:
 * - kContextFieldName: name of the field containing the context kind
 * - kReferenceFieldName: name of the field containing the attribute reference
 * or attribute name'
 *
 * To ensure the field names don't go out of sync with the declared member
 * variables, use the two macros defined below.
 * @tparam Fields
 */
template <typename T, typename = void>
struct ContextAwareReference {
    static_assert(
        std::is_same<char const* const,
                     decltype(T::kContextFieldName)>::value &&
            std::is_same<char const* const,
                         decltype(T::kReferenceFieldName)>::value,
        "T must define kContextFieldName and kReferenceFieldName as constexpr "
        "static const char*");
};

template <typename FieldNames>
struct ContextAwareReference<
    FieldNames,
    typename std::enable_if<
        std::is_same<char const* const,
                     decltype(FieldNames::kContextFieldName)>::value &&
        std::is_same<char const* const,
                     decltype(FieldNames::kReferenceFieldName)>::value>::type> {
    using fields = FieldNames;
    ContextKind contextKind;
    AttributeReference reference;
};

#define DEFINE_CONTEXT_KIND_FIELD(name) \
    ContextKind name;                   \
    constexpr static const char* kContextFieldName = #name;

#define DEFINE_ATTRIBUTE_REFERENCE_FIELD(name) \
    AttributeReference name;                   \
    constexpr static const char* kReferenceFieldName = #name;

}  // namespace launchdarkly::data_model
