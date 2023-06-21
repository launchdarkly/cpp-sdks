#pragma once

#include <boost/json/value.hpp>
#include <launchdarkly/data_model/item_descriptor.hpp>
#include <launchdarkly/data_model/segment.hpp>
#include <optional>
#include <tl/expected.hpp>
#include <unordered_map>

namespace launchdarkly::data_model {

struct SDKDataSet {
    using FlagKey = std::string;
    using SegmentKey = std::string;
    // std::unordered_map<FlagKey, ItemDescriptor<Flag>> flags;
    std::optional<
        std::unordered_map<SegmentKey, std::optional<ItemDescriptor<Segment>>>>
        segments;
};

}  // namespace launchdarkly::data_model
