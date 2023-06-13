#pragma once

#include <tl/expected.hpp>

#include <boost/json.hpp>

#include <launchdarkly/data_model/item_descriptor.hpp>
#include "json_errors.hpp"

namespace launchdarkly {

template <typename T>
tl::expected<std::unordered_map<std::string, data_model::ItemDescriptor<T>>,
             JsonError>
tag_invoke(boost::json::value_to_tag<tl::expected<
               std::unordered_map<std::string, data_model::ItemDescriptor<T>>,
               JsonError>> const& unused,
           boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (!json_value.is_object()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    auto const& obj = json_value.as_object();
    std::unordered_map<std::string, data_model::ItemDescriptor<T>> descriptors;
    for (auto const& pair : obj) {
        auto eval_result =
            boost::json::value_to<tl::expected<T, JsonError>>(pair.value());
        if (!eval_result.has_value()) {
            return tl::unexpected(JsonError::kSchemaFailure);
        }
        descriptors.emplace(pair.key(), data_model::ItemDescriptor<T>(
                                            std::move(eval_result.value())));
    }
    return descriptors;
}

template <typename T>
void tag_invoke(
    boost::json::value_from_tag const& unused,
    boost::json::value& json_value,
    std::unordered_map<std::string,
                       std::shared_ptr<data_model::ItemDescriptor<T>>> const&
        all_flags) {
    boost::ignore_unused(unused);

    auto& obj = json_value.emplace_object();
    for (auto descriptor : all_flags) {
        // Only serialize non-deleted items..
        if (descriptor.second->item) {
            auto eval_result_json =
                boost::json::value_from(*descriptor.second->item);
            obj.emplace(descriptor.first, eval_result_json);
        }
    }
}
}  // namespace launchdarkly
