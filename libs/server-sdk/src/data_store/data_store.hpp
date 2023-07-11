#pragma once

#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/item_descriptor.hpp>
#include <launchdarkly/data_model/segment.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_store {

/**
 * Interface for readonly access to SDK data.
 *
 * The data store is what the client uses to store feature flag data that has
 * been received from LaunchDarkly.
 */
class IDataStore {
   public:
    using FlagDescriptor = launchdarkly::data_model::ItemDescriptor<
        launchdarkly::data_model::Flag>;
    using SegmentDescriptor = launchdarkly::data_model::ItemDescriptor<
        launchdarkly::data_model::Segment>;

    /**
     * Get a flag from the store.
     *
     * @param key The key for the flag.
     * @return Returns a shared_ptr to the FlagDescriptor, or a nullptr if there
     * is no such flag or the flag was deleted.
     */
    [[nodiscard]] virtual std::shared_ptr<FlagDescriptor> GetFlag(
        std::string key) const = 0;

    /**
     * Get a segment from the store.
     *
     * @param key The key for the segment.
     * @return Returns a shared_ptr to the SegmentDescriptor, or a nullptr if
     * there is no such segment, or the segment was deleted.
     */
    [[nodiscard]] virtual std::shared_ptr<SegmentDescriptor> GetSegment(
        std::string key) const = 0;

    /**
     * Get all of the flags.
     *
     * @return Returns an unordered map of FlagDescriptors.
     */
    [[nodiscard]] virtual std::unordered_map<std::string, std::shared_ptr<FlagDescriptor>> AllFlags()
        const = 0;

    /**
     * Get all of the segments.
     *
     * @return Returns an unordered map of SegmentDescriptors.
     */
    [[nodiscard]] virtual std::unordered_map<std::string, std::shared_ptr<SegmentDescriptor>>
    AllSegments() const = 0;

    /**
     * Check if the store is initialized.
     *
     * @return Returns true if the store is initialized.
     */
    [[nodiscard]] virtual bool Initialized() const = 0;

    /**
     * Get a description of the store.
     * @return Returns a string containing a description of the store.
     */
    [[nodiscard]] virtual std::string const& Description() const = 0;

    IDataStore(IDataStore const& item) = delete;
    IDataStore(IDataStore&& item) = delete;
    IDataStore& operator=(IDataStore const&) = delete;
    IDataStore& operator=(IDataStore&&) = delete;
    virtual ~IDataStore() = default;

   protected:
    IDataStore() = default;
};

}  // namespace launchdarkly::server_side::data_store
