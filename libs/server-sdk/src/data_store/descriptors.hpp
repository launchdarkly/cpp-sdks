#pragma once

#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/item_descriptor.hpp>
#include <launchdarkly/data_model/segment.hpp>

namespace launchdarkly::server_side::data_store {
using FlagDescriptor =
    launchdarkly::data_model::ItemDescriptor<launchdarkly::data_model::Flag>;
using SegmentDescriptor =
    launchdarkly::data_model::ItemDescriptor<launchdarkly::data_model::Segment>;
}  // namespace launchdarkly::server_side::data_store
