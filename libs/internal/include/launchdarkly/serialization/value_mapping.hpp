#pragma once

#include <boost/json.hpp>
#include <launchdarkly/serialization/json_errors.hpp>
#include <launchdarkly/serialization/json_primitives.hpp>
#include <tl/expected.hpp>

#define PARSE_OPTIONAL_FIELD(field, obj, key)                            \
    do {                                                                 \
        if (auto error = ParseOptionalFieldOutParam(obj, key, &field)) { \
            return tl::make_unexpected(error.value());                   \
        }                                                                \
    } while (0)

#define PARSE_REQUIRED_FIELD(field, obj, key)                            \
    do {                                                                 \
        if (auto error = ParseRequiredFieldOutParam(obj, key, &field)) { \
            return tl::make_unexpected(error.value());                   \
        }                                                                \
    } while (0)

#define REQUIRE_OBJECT(value)                                      \
    do {                                                           \
        if (!json_value.is_object()) {                             \
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

template <typename T>
tl::expected<std::optional<T>, JsonError> ParseOptionalField(
    boost::json::object const& obj,
    std::string const& field_name) {
    auto const& it = obj.find(field_name);
    if (it == obj.end()) {
        return std::nullopt;
    }
    if (it->value().is_null()) {
        return std::nullopt;
    }
    auto val = boost::json::value_to<tl::expected<T, JsonError>>(it->value());
    if (!val) {
        return tl::make_unexpected(val.error());
    }
    return std::make_optional(val.value());
}

template <typename T>
std::optional<JsonError> ParseOptionalFieldOutParam(
    boost::json::object const& obj,
    std::string const& field_name,
    std::optional<T>* out_value) {
    auto maybe_opt_val = ParseOptionalField<T>(obj, field_name);
    if (!maybe_opt_val) {
        return maybe_opt_val.error();
    }
    *out_value = std::move(maybe_opt_val.value());
    return std::nullopt;
}

template <typename T>
tl::expected<T, JsonError> ParseRequiredField(boost::json::object const& obj,
                                              std::string const& field_name) {
    auto const& it = obj.find(field_name);
    if (it == obj.end()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    if (it->value().is_null()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    auto val = boost::json::value_to<tl::expected<T, JsonError>>(it->value());
    if (!val) {
        return tl::make_unexpected(val.error());
    }
    return val.value();
}

template <typename T>
std::optional<JsonError> ParseRequiredFieldOutParam(
    boost::json::object const& obj,
    std::string const& field_name,
    T* out_value) {
    auto maybe_val = ParseRequiredField<T>(obj, field_name);
    if (!maybe_val) {
        return maybe_val.error();
    }
    *out_value = std::move(maybe_val.value());
    return std::nullopt;
}

template <>
std::optional<uint64_t> ValueAsOpt(boost::json::object::const_iterator iterator,
                                   boost::json::object::const_iterator end);

template <>
std::optional<bool> ValueAsOpt(boost::json::object::const_iterator iterator,
                               boost::json::object::const_iterator end);

// Returns std::nullopt if:
// - The iterator value is not an array
// - ANY element of the array is not a string
template <>
std::optional<std::vector<std::string>> ValueAsOpt(
    boost::json::object::const_iterator iterator,
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
}  // namespace launchdarkly
