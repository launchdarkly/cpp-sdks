#pragma once

#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/item_descriptor.hpp>
#include <launchdarkly/data_model/segment.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_store {
using FlagDescriptor =
    launchdarkly::data_model::ItemDescriptor<launchdarkly::data_model::Flag>;
using SegmentDescriptor =
    launchdarkly::data_model::ItemDescriptor<launchdarkly::data_model::Segment>;

/**
 * Get a flag from the store.
 *
 * @param key The key for the flag.
 * @return Returns a shared_ptr to the FlagDescriptor, or a nullptr if there
 * is not such flag, or the flag was deleted.
 */
[[nodiscard]] std::shared_ptr<FlagDescriptor> GetFlag(std::string key) const;

/**
 * Get a segment from the store.
 * @param key The key for the segment.
 * @return Returns a shared_ptr to the SegmentDescriptor, or a nullptr if there
 * is no such segment, or the segment was deleted.
 */
[[nodiscard]] std::shared_ptr<SegmentDescriptor> GetSegment(std::string key) const;

/**
 * Get all of the flags.
 * @return Returns an unordered
 */
[[nodiscard]] std::unordered_map<std::string, FlagDescriptor> AllFlags() const;

/**
 *
 * @return
 */
[[nodiscard]] std::unordered_map<std::string, SegmentDescriptor> AllSegments() const;

/**
 *
 * @return
 */
[[nodiscard]] bool Initialized() const;

/**
 *
 * @return
 */
[[nodiscard]] std::string const& Description() const;

}  // namespace launchdarkly::server_side::data_store
