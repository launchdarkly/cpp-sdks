#pragma once

#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/item_descriptor.hpp>
#include <launchdarkly/data_model/segment.hpp>
#include <mutex>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::flag_manager {

using FlagItemDescriptor = data_model::ItemDescriptor<data_model::Flag>;
using SegmentItemDescriptor = data_model::ItemDescriptor<data_model::Segment>;

class FlagStore {
   public:
    using FlagItem = std::shared_ptr<FlagItemDescriptor>;
    using SegmentItem = std::shared_ptr<SegmentItemDescriptor>;

    void Init(
        std::unordered_map<std::string, FlagItemDescriptor> const& flags,
        std::unordered_map<std::string, SegmentItemDescriptor> const& segments);

    void Upsert(std::string const& key, FlagItemDescriptor item);
    void Upsert(std::string const& key, SegmentItemDescriptor item);

    /**
     * Attempts to get a flag by key from the current flags.
     *
     * @param flag_key The flag to get.
     * @return A shared_ptr to the value if present. A null shared_ptr if the
     * item is not present.
     */
    [[nodiscard]] FlagItem GetFlag(std::string const& flag_key) const;

    /**
     * Attempts to get a segment by key from the current segments.
     *
     * @param flag_key The segment to get.
     * @return A shared_ptr to the value if present. A null shared_ptr if the
     * item is not present.
     */
    [[nodiscard]] SegmentItem GetSegment(std::string const& segment_key) const;

    /**
     * Gets all the current flags.
     *
     * @return All of the current flags.
     */
    [[nodiscard]] std::unordered_map<std::string, FlagItem> GetAll() const;

   private:
    void UpdateData(
        std::unordered_map<std::string, FlagItemDescriptor> const& flags,
        std::unordered_map<std::string, SegmentItemDescriptor> const& segments);

    std::unordered_map<std::string, FlagItem> flags_;
    std::unordered_map<std::string, SegmentItem> segments_;

    mutable std::mutex data_mutex_;
};

}  // namespace launchdarkly::server_side::flag_manager
