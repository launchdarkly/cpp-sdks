#pragma once
#include "descriptors.hpp"

#include <launchdarkly/data_model/sdk_data_set.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_sources {

class ISynchronizer;
class IBootstrapper;
class IDataDestination;

class IDataSource {
   public:
    [[nodiscard]] virtual std::string const& Identity() const;

    [[nodiscard]] virtual ISynchronizer* GetSynchronizer();
    [[nodiscard]] virtual IBootstrapper* GetBootstrapper();

    // TODO: Have a GetDataStore() interface? That way we don't need to forward
    // methods.

    [[nodiscard]] virtual std::shared_ptr<FlagDescriptor> GetFlag(
        std::string const& key) const = 0;

    /**
     * Get a segment from the store.
     *
     * @param key The key for the segment.
     * @return Returns a shared_ptr to the SegmentDescriptor, or a nullptr if
     * there is no such segment, or the segment was deleted.
     */
    [[nodiscard]] virtual std::shared_ptr<SegmentDescriptor> GetSegment(
        std::string const& key) const = 0;

    /**
     * Get all of the flags.
     *
     * @return Returns an unordered map of FlagDescriptors.
     */
    [[nodiscard]] virtual std::unordered_map<std::string,
                                             std::shared_ptr<FlagDescriptor>>
    AllFlags() const = 0;

    /**
     * Get all of the segments.
     *
     * @return Returns an unordered map of SegmentDescriptors.
     */
    [[nodiscard]] virtual std::unordered_map<std::string,
                                             std::shared_ptr<SegmentDescriptor>>
    AllSegments() const = 0;

    virtual ~IDataSource() = default;
    IDataSource(IDataSource const& item) = delete;
    IDataSource(IDataSource&& item) = delete;
    IDataSource& operator=(IDataSource const&) = delete;
    IDataSource& operator=(IDataSource&&) = delete;

   protected:
    IDataSource() = default;
};

}  // namespace launchdarkly::server_side::data_sources
