#pragma once

#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/item_descriptor.hpp>
#include <launchdarkly/data_model/segment.hpp>

#include <boost/json/value.hpp>
#include <tl/expected.hpp>

#include <optional>
#include <unordered_map>

namespace launchdarkly::data_model {

struct SDKDataSet {
    using FlagKey = std::string;
    using SegmentKey = std::string;
    std::unordered_map<FlagKey, ItemDescriptor<Flag>> flags;
    std::unordered_map<SegmentKey, ItemDescriptor<Segment>> segments;
};

}  // namespace launchdarkly::data_model
