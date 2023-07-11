#pragma once

#include "descriptors.hpp"

#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/item_descriptor.hpp>
#include <launchdarkly/data_model/segment.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_store {

/**
 * Interface for readonly access to SDK data.
 */
class IDataStore {
   public:
    /**
     * Get a flag from the store.
     *
     * @param key The key for the flag.
     * @return Returns a shared_ptr to the FlagDescriptor, or a nullptr if there
     * is no such flag or the flag was deleted.
     */
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
        std::string const&  key) const = 0;

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
