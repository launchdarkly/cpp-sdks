#pragma once

#include <boost/json/value.hpp>
#include <launchdarkly/data_model/item_descriptor.hpp>
#include <launchdarkly/data_model/segment.hpp>
#include <tl/expected.hpp>
#include <unordered_map>

namespace launchdarkly::data_model {

struct SDKDataSet {
    using FlagKey = std::string;
    using SegmentKey = std::string;
    // std::unordered_map<FlagKey, ItemDescriptor<Flag>> flags;
    std::unordered_map<SegmentKey, ItemDescriptor<Segment>> segments;
};

tl::expected<SDKDataSet, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<SDKDataSet, JsonError>> const&
        unused,
    boost::json::value const& json_value);

}  // namespace launchdarkly::data_model
