#pragma once

#include <launchdarkly/data_model/descriptors.hpp>
#include <launchdarkly/data_model/sdk_data_set.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_retrieval {

class IDataSystem {
   public:
    [[nodiscard]] virtual std::string const& Identity() const;

    virtual void Initialize();

    [[nodiscard]] virtual std::shared_ptr<data_model::FlagDescriptor> GetFlag(
        std::string const& key) const = 0;

    /**
     * Get a segment from the store.
     *
     * @param key The key for the segment.
     * @return Returns a shared_ptr to the SegmentDescriptor, or a nullptr if
     * there is no such segment, or the segment was deleted.
     */
    [[nodiscard]] virtual std::shared_ptr<data_model::SegmentDescriptor>
    GetSegment(std::string const& key) const = 0;

    /**
     * Get all of the flags.
     *
     * @return Returns an unordered map of FlagDescriptors.
     */
    [[nodiscard]] virtual std::
        unordered_map<std::string, std::shared_ptr<data_model::FlagDescriptor>>
        AllFlags() const = 0;

    /**
     * Get all of the segments.
     *
     * @return Returns an unordered map of SegmentDescriptors.
     */
    [[nodiscard]] virtual std::unordered_map<
        std::string,
        std::shared_ptr<data_model::SegmentDescriptor>>
    AllSegments() const = 0;

    virtual ~IDataSystem() = default;
    IDataSystem(IDataSystem const& item) = delete;
    IDataSystem(IDataSystem&& item) = delete;
    IDataSystem& operator=(IDataSystem const&) = delete;
    IDataSystem& operator=(IDataSystem&&) = delete;

   protected:
    IDataSystem() = default;
};

}  // namespace launchdarkly::server_side::data_retrieval
