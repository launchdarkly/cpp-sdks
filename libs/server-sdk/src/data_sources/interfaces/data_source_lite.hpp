#pragma once
#include "../descriptors.hpp"

#include <launchdarkly/data_model/sdk_data_set.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_sources {

class IDataSourceLite {
   public:
    [[nodiscard]] virtual FlagDescriptor GetFlag(
        std::string const& key) const = 0;

    /**
     * Get a segment from the store.
     *
     * @param key The key for the segment.
     * @return Returns a shared_ptr to the SegmentDescriptor, or a nullptr if
     * there is no such segment, or the segment was deleted.
     */
    [[nodiscard]] virtual SegmentDescriptor GetSegment(
        std::string const& key) const = 0;

    /**
     * Get all of the flags.
     *
     * @return Returns an unordered map of FlagDescriptors.
     */
    [[nodiscard]] virtual std::unordered_map<std::string, FlagDescriptor>
    AllFlags() const = 0;

    /**
     * Get all of the segments.
     *
     * @return Returns an unordered map of SegmentDescriptors.
     */
    [[nodiscard]] virtual std::unordered_map<std::string, SegmentDescriptor>
    AllSegments() const = 0;

    [[nodiscard]] virtual std::string const& Identity() const = 0;

    virtual ~IDataSourceLite() = default;
    IDataSourceLite(IDataSourceLite const& item) = delete;
    IDataSourceLite(IDataSourceLite&& item) = delete;
    IDataSourceLite& operator=(IDataSourceLite const&) = delete;
    IDataSourceLite& operator=(IDataSourceLite&&) = delete;

   protected:
    IDataSourceLite() = default;
};

}  // namespace launchdarkly::server_side::data_sources
