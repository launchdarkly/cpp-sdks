#pragma once

#include <boost/json.hpp>
#include <launchdarkly/serialization/json_errors.hpp>
#include <launchdarkly/serialization/json_primitives.hpp>
#include <tl/expected.hpp>

#define PARSE_FIELD(field, it)                                               \
    if (auto result =                                                        \
            boost::json::value_to<tl::expected<decltype(field), JsonError>>( \
                it->value())) {                                              \
        field = result.value();                                              \
    } else {                                                                 \
        return tl::make_unexpected(result.error());                          \
    }

// Attempts to parse a field only if it exists in the data. Propagates an error
// if the field's destination type is not compatible with the data.
#define PARSE_OPTIONAL_FIELD(field, obj, key) \
    do {                                      \
        auto const& it = obj.find(key);       \
        if (it != obj.end()) {                \
            PARSE_FIELD(field, it);           \
        }                                     \
    } while (0)

// Propagates an error upwards if the specified field isn't present in the
// data.
#define PARSE_REQUIRED_FIELD(field, obj, key)                      \
    do {                                                           \
        auto const& it = obj.find(key);                            \
        if (it == obj.end()) {                                     \
            return tl::make_unexpected(JsonError::kSchemaFailure); \
        }                                                          \
        PARSE_FIELD(field, it);                                    \
    } while (0)

#define REQUIRE_OBJECT(value)                                      \
    do {                                                           \
        if (json_value.is_null()) {                                \
            return std::nullopt;                                   \
        }                                                          \
        if (!json_value.is_object()) {                             \
            return tl::make_unexpected(JsonError::kSchemaFailure); \
        }                                                          \
        if (json_value.as_object().empty()) {                      \
            return std::nullopt;                                   \
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
        if (json_value.as_string().empty()) {                      \
            return std::nullopt;                                   \
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
}  // namespace launchdarkly
