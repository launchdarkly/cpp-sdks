#pragma once

#include <data_interfaces/store/istore.hpp>
#include <launchdarkly/data_model/descriptors.hpp>
#include <memory>

namespace launchdarkly::server_side::test_store {

/**
 * @return A data store preloaded with flags/segments for unit tests.
 */
std::unique_ptr<data_interfaces::IStore> TestData();

/**
 * @return An initialized, but empty, data store.
 */
std::unique_ptr<data_interfaces::IStore> Empty();

/**
 * Returns a flag suitable for inserting into a memory store, parsed from the
 * given JSON representation.
 */
data_model::FlagDescriptor Flag(char const* json);

/**
 * Returns a segment suitable for inserting into a memory store, parsed from the
 * given JSON representation.
 */
data_model::SegmentDescriptor Segment(char const* json);

}  // namespace launchdarkly::server_side::test_store
