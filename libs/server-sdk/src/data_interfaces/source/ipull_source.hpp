#pragma once

#include <launchdarkly/data_model/descriptors.hpp>
#include <launchdarkly/data_model/sdk_data_set.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_interfaces {

class IPullSource {
   public:
    template <typename T>
    using ItemResult = tl::expected<std::optional<T>, std::string>;

    template <typename T>
    using AllResult =
        tl::expected<std::unordered_map<std::string, T>, std::string>;

    [[nodiscard]] virtual ItemResult<data_model::FlagDescriptor> GetFlag(
        std::string const& key) const = 0;

    /**
     * Get a segment from the store.
     *
     * @param key The key for the segment.
     * @return Returns a shared_ptr to the SegmentDescriptor, or a nullptr if
     * there is no such segment, or the segment was deleted.
     */
    [[nodiscard]] virtual ItemResult<data_model::SegmentDescriptor> GetSegment(
        std::string const& key) const = 0;

    /**
     * Get all of the flags.
     *
     * @return Returns an unordered map of FlagDescriptors.
     */
    [[nodiscard]] virtual AllResult<data_model::FlagDescriptor> AllFlags()
        const = 0;

    /**
     * Get all of the segments.
     *
     * @return Returns an unordered map of SegmentDescriptors.
     */
    [[nodiscard]] virtual AllResult<data_model::SegmentDescriptor> AllSegments()
        const = 0;

    [[nodiscard]] virtual std::string const& Identity() const = 0;

    [[nodiscard]] virtual bool Initialized() const = 0;

    virtual ~IPullSource() = default;
    IPullSource(IPullSource const& item) = delete;
    IPullSource(IPullSource&& item) = delete;
    IPullSource& operator=(IPullSource const&) = delete;
    IPullSource& operator=(IPullSource&&) = delete;

   protected:
    IPullSource() = default;
};

}  // namespace launchdarkly::server_side::data_interfaces
