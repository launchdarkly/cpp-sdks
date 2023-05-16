#pragma once

#include <unordered_map>

#include <boost/json.hpp>

#include <tl/expected.hpp>

#include "../data_sources/data_source_update_sink.hpp"

#include <launchdarkly/serialization/json_errors.hpp>

namespace launchdarkly::client_side {

static tl::expected<
    std::unordered_map<std::string, launchdarkly::client_side::ItemDescriptor>,
    JsonError>
tag_invoke(boost::json::value_to_tag<tl::expected<
               std::unordered_map<std::string,
                                  launchdarkly::client_side::ItemDescriptor>,
               JsonError>> const& unused,
           boost::json::value const& json_value);

void tag_invoke(
    boost::json::value_from_tag const& unused,
    boost::json::value& json_value,
    std::unordered_map<std::string, std::shared_ptr<ItemDescriptor>> const&
        evaluation_result);

}  // namespace launchdarkly::client_side
