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
    [[nodiscard]] virtual std::string Identity() const;

    [[nodiscard]] virtual std::weak_ptr<ISynchronizer> GetSynchronizer() const;
    [[nodiscard]] virtual std::weak_ptr<IBootstrapper> GetBootstrapper() const;

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

class ISynchronizer {
   public:
    virtual void Init(std::optional<data_model::SDKDataSet> initial_data,
                      IDataDestination& destination) = 0;
    virtual void Start() = 0;
    virtual void ShutdownAsync(std::function<void()>) = 0;
    virtual ~ISynchronizer() = default;
    ISynchronizer(ISynchronizer const& item) = delete;
    ISynchronizer(ISynchronizer&& item) = delete;
    ISynchronizer& operator=(ISynchronizer const&) = delete;
    ISynchronizer& operator=(ISynchronizer&&) = delete;

   protected:
    ISynchronizer() = default;
};

class IBootstrapper {
   public:
    using Error = std::string;
    virtual bool IsAuthoritative() const = 0;
    virtual tl::expected<data_model::SDKDataSet, Error> FetchAll(
        std::chrono::milliseconds timeout_hint) = 0;
    virtual ~IBootstrapper() = default;
    IBootstrapper(IBootstrapper const& item) = delete;
    IBootstrapper(IBootstrapper&& item) = delete;
    IBootstrapper& operator=(IBootstrapper const&) = delete;
    IBootstrapper& operator=(IBootstrapper&&) = delete;

   protected:
    IBootstrapper() = default;
};

class IDataDestination {
   public:
    virtual ~IDataDestination() = default;
    IDataDestination(IDataDestination const& item) = delete;
    IDataDestination(IDataDestination&& item) = delete;
    IDataDestination& operator=(IDataDestination const&) = delete;
    IDataDestination& operator=(IDataDestination&&) = delete;

   protected:
    IDataDestination() = default;
};
}  // namespace launchdarkly::server_side::data_sources
