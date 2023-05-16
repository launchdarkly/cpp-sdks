#pragma once

#include <unordered_map>

#include <boost/json.hpp>

#include "../data_sources/data_source_update_sink.hpp"

namespace launchdarkly::client_side::serialization {

void tag_invoke(
    boost::json::value_from_tag const& unused,
    boost::json::value& json_value,
    std::unordered_map<std::string, std::shared_ptr<ItemDescriptor>> const&
        evaluation_result);

}  // namespace launchdarkly::client_side::serialization
