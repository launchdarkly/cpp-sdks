#pragma once

#include <launchdarkly/data_model/descriptors.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_interfaces {

/**
 * \brief IStore provides shared ownership of flag and segment domain
 * objects.
 */
class IStore {
   public:
    /**
     * \brief Get the flag named by key. Returns nullptr if no such flag exists.
     * \param key Key of the flag.
     * \return Shared pointer to the flag.
     */
    [[nodiscard]] virtual std::shared_ptr<data_model::FlagDescriptor> GetFlag(
        std::string const& key) const = 0;

    /**
     * \brief Get the segment named by key. Returns nullptr if no such flag
     * exists. \param key Key of the segment. \return Shared pointer to the
     * segment.
     */
    [[nodiscard]] virtual std::shared_ptr<data_model::SegmentDescriptor>
    GetSegment(std::string const& key) const = 0;

    /**
     * \brief Get a map of all flags.
     * \return Map of shared pointers to flags.
     */
    [[nodiscard]] virtual std::
        unordered_map<std::string, std::shared_ptr<data_model::FlagDescriptor>>
        AllFlags() const = 0;

    /**
     * \brief Get a map of all segments.
     * \return Map of shared pointers to segments.
     */
    [[nodiscard]] virtual std::unordered_map<
        std::string,
        std::shared_ptr<data_model::SegmentDescriptor>>
    AllSegments() const = 0;

    /**
     * @return True if the store has ever contained data.
     */
    [[nodiscard]] virtual bool Initialized() const = 0;

    virtual ~IStore() = default;
    IStore(IStore const& item) = delete;
    IStore(IStore&& item) = delete;
    IStore& operator=(IStore const&) = delete;
    IStore& operator=(IStore&&) = delete;

   protected:
    IStore() = default;
};
}  // namespace launchdarkly::server_side::data_interfaces
