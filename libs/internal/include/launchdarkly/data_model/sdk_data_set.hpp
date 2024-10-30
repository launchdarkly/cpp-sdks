#pragma once

#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/item_descriptor.hpp>
#include <launchdarkly/data_model/segment.hpp>

#include <string>
#include <unordered_map>

namespace launchdarkly::data_model {

struct SDKDataSet {
    template <typename KeyType, typename Storage>
    using Collection = std::unordered_map<KeyType, ItemDescriptor<Storage>>;
    using FlagKey = std::string;
    using SegmentKey = std::string;
    using Flags = Collection<FlagKey, Flag>;
    using Segments = Collection<SegmentKey, Segment>;

    Flags flags;
    Segments segments;
};

}  // namespace launchdarkly::data_model
