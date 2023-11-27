#pragma once

#include <launchdarkly/serialization/json_errors.hpp>
#include <launchdarkly/serialization/json_primitives.hpp>

#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>
#include <tl/expected.hpp>

#include <optional>
#include <type_traits>

// Parses a field, propagating an error if the field's value is of the wrong
// type. If the field was null or omitted in the data, it is set to
// default_value.
#define PARSE_FIELD_DEFAULT(field, obj, key, default_value) \
    do {                                                    \
        std::optional<decltype(field)> maybe_val;           \
        PARSE_CONDITIONAL_FIELD(maybe_val, obj, key);       \
        field = maybe_val.value_or(default_value);          \
    } while (0)

// Parses a field, propagating an error if the field's value is of the wrong
// type. Intended for fields where the "zero value" of that field is a valid
// member of the domain of that field. If the "zero value" of the field is meant
// to denote absence of that field, rather than a valid member of the domain,
// then use PARSE_CONDITIONAL_FIELD in order to avoid discarding the information
// of whether that field was present or not.
#define PARSE_FIELD(field, obj, key)                                      \
    do {                                                                  \
        static_assert(std::is_default_constructible_v<decltype(field)> && \
                      "field must be default-constructible");             \
        PARSE_FIELD_DEFAULT(field, obj, key, decltype(field){});          \
    } while (0)

// Parses a field that is conditional and/or has no valid default value.
// This would be the case for fields that depend on the existence of some other
// field. Another scenario would be a string field representing enum values,
// where there's no default defined/empty string is meaningless. It will
// propagate an error if the field's value is of the wrong type. Intended to be
// called on fields of type std::optional<T>.
#define PARSE_CONDITIONAL_FIELD(field, obj, key)                              \
    do {                                                                      \
        auto const& it = obj.find(key);                                       \
        if (it != obj.end()) {                                                \
            if (auto result = boost::json::value_to<                          \
                    tl::expected<decltype(field), JsonError>>(it->value())) { \
                field = result.value();                                       \
            } else {                                                          \
                /* Field was of wrong type. */                                \
                return tl::make_unexpected(result.error());                   \
            }                                                                 \
        }                                                                     \
    } while (0)

// Parses a field, propagating an error if it is omitted/null or if the field's
// value is of the wrong type. Use only if the field *must* be present in the
// JSON document. Think twice; this is unlikely - most fields have a
// well-defined default value that can be used if not present.
#define PARSE_REQUIRED_FIELD(field, obj, key)                             \
    do {                                                                  \
        auto const& it = obj.find(key);                                   \
        if (it == obj.end()) {                                            \
            /* Ideally report that field is missing, instead of generic   \
             * failure */                                                 \
            return tl::make_unexpected(JsonError::kSchemaFailure);        \
        }                                                                 \
        auto result = boost::json::value_to<                              \
            tl::expected<std::optional<decltype(field)>, JsonError>>(     \
            it->value());                                                 \
        if (!result) {                                                    \
            /* The field's value is of the wrong type. */                 \
            return tl::make_unexpected(result.error());                   \
        }                                                                 \
        /* We have the field, but its value might be null. */             \
        auto const& maybe_val = result.value();                           \
        if (!maybe_val) {                                                 \
            /* Ideally report that the field was null, instead of generic \
             * failure. */                                                \
            return tl::make_unexpected(JsonError::kSchemaFailure);        \
        }                                                                 \
        field = std::move(*maybe_val);                                    \
    } while (0)

#define REQUIRE_OBJECT(value)                                      \
    do {                                                           \
        if (json_value.is_null()) {                                \
            return std::nullopt;                                   \
        }                                                          \
        if (!json_value.is_object()) {                             \
            return tl::make_unexpected(JsonError::kSchemaFailure); \
        }                                                          \
    } while (0)

#define REQUIRE_STRING(value)                                      \
    do {                                                           \
        if (json_value.is_null()) {                                \
            return std::nullopt;                                   \
        }                                                          \
        if (!json_value.is_string()) {                             \
            return tl::make_unexpected(JsonError::kSchemaFailure); \
        }                                                          \
    } while (0)

namespace launchdarkly {
template <typename Type>
std::optional<Type> ValueAsOpt(boost::json::object::const_iterator iterator,
                               boost::json::object::const_iterator end) {
    boost::ignore_unused(iterator);
    boost::ignore_unused(end);

    static_assert(sizeof(Type) == -1, "Must be specialized to use ValueAsOpt");
}

template <typename Type>
Type ValueOrDefault(boost::json::object::const_iterator iterator,
                    boost::json::object::const_iterator end,
                    Type default_value) {
    boost::ignore_unused(iterator);
    boost::ignore_unused(end);
    boost::ignore_unused(default_value);

    static_assert(sizeof(Type) == -1,
                  "Must be specialized to use ValueOrDefault");
}

template <typename OutType, typename InType>
std::optional<OutType> MapOpt(std::optional<InType> opt,
                              std::function<OutType(InType&)> const& mapper) {
    if (opt.has_value()) {
        return std::make_optional(mapper(opt.value()));
    }
    return std::nullopt;
}

template <>
std::optional<uint64_t> ValueAsOpt(boost::json::object::const_iterator iterator,
                                   boost::json::object::const_iterator end);

template <>
std::optional<std::string> ValueAsOpt(
    boost::json::object::const_iterator iterator,
    boost::json::object::const_iterator end);

template <>
bool ValueOrDefault(boost::json::object::const_iterator iterator,
                    boost::json::object::const_iterator end,
                    bool default_value);

template <>
uint64_t ValueOrDefault(boost::json::object::const_iterator iterator,
                        boost::json::object::const_iterator end,
                        uint64_t default_value);

template <>
std::string ValueOrDefault(boost::json::object::const_iterator iterator,
                           boost::json::object::const_iterator end,
                           std::string default_value);

template <typename T>
void WriteMinimal(boost::json::object& obj,
                  std::string const& key,  // No copy when not used.
                  std::optional<T> val) {
    if (val.has_value()) {
        obj.emplace(key, boost::json::value_from(val.value()));
    }
}

template <typename T>
void WriteMinimal(boost::json::object& obj,
                  std::string const& key,
                  std::vector<T> const& val) {
    if (!val.empty()) {
        obj.emplace(key, boost::json::value_from(val));
    }
}

template <typename T>
void WriteMinimal(boost::json::object& obj,
                  std::string const& key,
                  T const& val,
                  std::function<bool()> const& predicate) {
    if (predicate()) {
        obj.emplace(key, boost::json::value_from(val));
    }
}

void WriteMinimal(boost::json::object& obj,
                  std::string const& key,  // No copy when not used.
                  bool val);

}  // namespace launchdarkly
