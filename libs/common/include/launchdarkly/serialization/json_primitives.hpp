#pragma once

#include <launchdarkly/serialization/json_errors.hpp>
#include <tl/expected.hpp>

#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>

#include <optional>
#include <unordered_map>

namespace launchdarkly {

template <typename T>
tl::expected<std::optional<std::vector<T>>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<std::vector<T>>, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (json_value.is_null()) {
        return std::nullopt;
    }

    if (!json_value.is_array()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }

    auto const& arr = json_value.as_array();
    std::vector<T> items;
    items.reserve(arr.size());
    for (auto const& item : arr) {
        auto eval_result =
            boost::json::value_to<tl::expected<std::optional<T>, JsonError>>(
                item);
        if (!eval_result.has_value()) {
            return tl::unexpected(eval_result.error());
        }
        auto maybe_val = eval_result.value();
        if (maybe_val) {
            items.emplace_back(std::move(maybe_val.value()));
        }
    }
    return items;
}

tl::expected<std::optional<bool>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<bool>, JsonError>> const& unused,
    boost::json::value const& json_value);

tl::expected<std::optional<std::uint64_t>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<std::uint64_t>, JsonError>> const& unused,
    boost::json::value const& json_value);

tl::expected<std::optional<std::int64_t>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<std::int64_t>, JsonError>> const& unused,
    boost::json::value const& json_value);

tl::expected<std::optional<std::string>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<std::string>, JsonError>> const& unused,
    boost::json::value const& json_value);

template <typename K, typename V>
tl::expected<std::optional<std::unordered_map<K, V>>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<std::unordered_map<K, V>>, JsonError>> const&
        unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (json_value.is_null()) {
        return std::nullopt;
    }
    if (!json_value.is_object()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    auto const& obj = json_value.as_object();
    std::unordered_map<K, V> descriptors;
    for (auto const& pair : obj) {
        auto eval_result =
            boost::json::value_to<tl::expected<std::optional<V>, JsonError>>(
                pair.value());
        if (!eval_result) {
            return tl::unexpected(eval_result.error());
        }
        auto const& maybe_val = eval_result.value();
        if (maybe_val) {
            descriptors.emplace(pair.key(), std::move(maybe_val.value()));
        }
    }
    return descriptors;
}

/**
 * Convenience implementation that deserializes a T via the tag_invoke overload
 * for std::optional<T>.
 *
 * If that overload returns std::nullopt, this returns
 * a default-constructed T.
 *
 * Json errors are propagated.
 */
template <typename T>
tl::expected<T, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<T, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    auto maybe_val =
        boost::json::value_to<tl::expected<std::optional<T>, JsonError>>(
            json_value);
    if (!maybe_val.has_value()) {
        return tl::unexpected(maybe_val.error());
    }
    return maybe_val.value().value_or(T());
}

}  // namespace launchdarkly
