#pragma once

#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>
#include <tl/expected.hpp>
#include "json_errors.hpp"

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

    if (json_value.as_array().empty()) {
        return std::nullopt;
    }

    auto const& arr = json_value.as_array();
    std::vector<T> items;
    items.reserve(arr.size());
    for (auto const& item : arr) {
        auto eval_result =
            boost::json::value_to<tl::expected<T, JsonError>>(item);
        if (!eval_result.has_value()) {
            return tl::unexpected(eval_result.error());
        }
        items.emplace_back(std::move(eval_result.value()));
    }
    return items;
}

template <typename T>
tl::expected<std::vector<T>, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<std::vector<T>, JsonError>> const&
        unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    auto maybe_val = boost::json::value_to<
        tl::expected<std::optional<std::vector<T>>, JsonError>>(json_value);
    if (!maybe_val.has_value()) {
        return tl::unexpected(maybe_val.error());
    }
    return maybe_val.value().value_or(std::vector<T>{});
}

tl::expected<std::optional<bool>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<bool>, JsonError>> const& unused,
    boost::json::value const& json_value);

tl::expected<bool, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<bool, JsonError>> const& unused,
    boost::json::value const& json_value);

tl::expected<std::optional<std::uint64_t>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<std::uint64_t>, JsonError>> const& unused,
    boost::json::value const& json_value);

tl::expected<std::uint64_t, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<std::uint64_t, JsonError>> const&
        unused,
    boost::json::value const& json_value);

tl::expected<std::optional<std::string>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<std::string>, JsonError>> const& unused,
    boost::json::value const& json_value);

tl::expected<std::string, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<std::string, JsonError>> const&
        unused,
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
    if (json_value.as_object().empty()) {
        return std::nullopt;
    }
    auto const& obj = json_value.as_object();
    std::unordered_map<K, V> descriptors;
    for (auto const& pair : obj) {
        auto eval_result =
            boost::json::value_to<tl::expected<V, JsonError>>(pair.value());
        if (!eval_result.has_value()) {
            return tl::unexpected(eval_result.error());
        }
        descriptors.emplace(pair.key(), eval_result.value());
    }
    return descriptors;
}

template <typename K, typename V>
tl::expected<std::unordered_map<K, V>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::unordered_map<K, V>, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    auto maybe_val = boost::json::value_to<
        tl::expected<std::optional<std::unordered_map<K, V>>, JsonError>>(
        json_value);
    if (!maybe_val.has_value()) {
        return tl::unexpected(maybe_val.error());
    }
    auto const& val = maybe_val.value();
    return val.value_or(std::unordered_map<K, V>{});
}

}  // namespace launchdarkly
