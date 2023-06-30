#pragma once

#include <tl/expected.hpp>

#include <boost/json.hpp>

#include <launchdarkly/data_model/item_descriptor.hpp>
#include <launchdarkly/serialization/json_primitives.hpp>
#include <launchdarkly/serialization/value_mapping.hpp>
#include "json_errors.hpp"

namespace launchdarkly {

template <typename T>
tl::expected<std::optional<data_model::ItemDescriptor<T>>, JsonError>
tag_invoke(boost::json::value_to_tag<
               tl::expected<std::optional<data_model::ItemDescriptor<T>>,
                            JsonError>> const& unused,
           boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    auto maybe_item =
        boost::json::value_to<tl::expected<std::optional<T>, JsonError>>(
            json_value);

    if (!maybe_item) {
        return tl::unexpected(maybe_item.error());
    }

    auto const& item = maybe_item.value();

    if (!item) {
        return std::nullopt;
    }

    return data_model::ItemDescriptor<T>(std::move(item.value()));
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
