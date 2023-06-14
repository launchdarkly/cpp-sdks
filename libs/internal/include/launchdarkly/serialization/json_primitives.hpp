#pragma once

#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>
#include <tl/expected.hpp>
#include "json_errors.hpp"

namespace launchdarkly {

template <typename T>
tl::expected<std::vector<T>, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<std::vector<T>, JsonError>> const&
        unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (!json_value.is_array()) {
        return tl::unexpected(JsonError::kSchemaFailure);
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

tl::expected<bool, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<bool, JsonError>> const& unused,
    boost::json::value const& json_value);

tl::expected<std::uint64_t, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<std::uint64_t, JsonError>> const&
        unused,
    boost::json::value const& json_value);

tl::expected<std::string, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<std::string, JsonError>> const&
        unused,
    boost::json::value const& json_value);
}  // namespace launchdarkly
