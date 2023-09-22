#pragma once

#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/item_descriptor.hpp>
#include <launchdarkly/data_model/segment.hpp>

namespace launchdarkly::data_model {
using FlagDescriptor = ItemDescriptor<Flag>;
using SegmentDescriptor = ItemDescriptor<Segment>;
}  // namespace launchdarkly::data_model
