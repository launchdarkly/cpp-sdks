#pragma once

#include "../../data_components/expiration_tracker/expiration_tracker.hpp"
#include "../../data_components/kinds/kinds.hpp"
#include "../../data_components/memory_store/memory_store.hpp"
#include "../../data_components/status_notifications/data_source_status_manager.hpp"
#include "../../data_interfaces/system/isystem.hpp"

#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/data_model/descriptors.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/server_side/integrations/serialized_descriptors.hpp>

#include <boost/asio/any_io_executor.hpp>

namespace launchdarkly::server_side::data_systems {

/**
 * LazyLoad implements a Data System that pulls data from a persistent store
 * on-demand. It is intended for use cases where holding an entire environment
 * in memory isn't desired, or for uses that disallow direct communication
 * with LaunchDarkly servers.
 *
 * LazyLoad is able to remain efficient because it caches responses from the
 * store. Over time, data becomes stale causing the system to refresh data.
 */
class LazyLoad : public data_interfaces::ISystem {
   public:
    LazyLoad(config::shared::built::ServiceEndpoints const& endpoints,
             config::shared::built::DataSourceConfig<
                 config::shared::ServerSDK> const& data_source_config,
             config::shared::built::HttpProperties http_properties,
             boost::asio::any_io_executor ioc,
             data_components::DataSourceStatusManager& status_manager,
             Logger const& logger);

    LazyLoad(LazyLoad const& item) = delete;
    LazyLoad(LazyLoad&& item) = delete;
    LazyLoad& operator=(LazyLoad const&) = delete;
    LazyLoad& operator=(LazyLoad&&) = delete;

    std::string const& Identity() const override;

    std::shared_ptr<data_model::FlagDescriptor> GetFlag(
        std::string const& key) const override;
    std::shared_ptr<data_model::SegmentDescriptor> GetSegment(
        std::string const& key) const override;
    std::unordered_map<std::string, std::shared_ptr<data_model::FlagDescriptor>>
    AllFlags() const override;
    std::unordered_map<std::string,
                       std::shared_ptr<data_model::SegmentDescriptor>>
    AllSegments() const override;

    bool Initialized() const;

    void Initialize() override;

   private:
    void RefreshAllFlags() const;
    void RefreshAllSegments() const;
    void RefreshInitState() const;
    void RefreshFlag(std::string const& key) const;
    void RefreshSegment(std::string const& key) const;

    static integrations::SerializedItemDescriptor Serialize(
        data_model::FlagDescriptor flag);
    static integrations::SerializedItemDescriptor Serialize(
        data_model::SegmentDescriptor segment);

    static std::optional<data_model::FlagDescriptor> DeserializeFlag(
        integrations::SerializedItemDescriptor flag);

    static std::optional<data_model::SegmentDescriptor> DeserializeSegment(
        integrations::SerializedItemDescriptor segment);

    template <typename TResult>
    static TResult Get(data_components::ExpirationTracker::TrackState state,
                       std::function<void(void)> refresh,
                       std::function<TResult(void)> get) {
        switch (state) {
            case data_components::ExpirationTracker::TrackState::kStale:
                [[fallthrough]];
            case data_components::ExpirationTracker::TrackState::kNotTracked:
                refresh();
                [[fallthrough]];
            case data_components::ExpirationTracker::TrackState::kFresh:
                return get();
        }
    }

    mutable data_components::MemoryStore memory_store_;
    std::shared_ptr<persistence::IPersistentStoreCore> core_;
    mutable data_components::ExpirationTracker tracker_;
    std::function<std::chrono::time_point<std::chrono::steady_clock>()> time_;
    mutable std::optional<bool> initialized_;

    struct Kinds {
        static data_components::FlagKind const Flag;
        static data_components::SegmentKind const Segment;
    };

    struct Keys {
        static inline std::string const kAllFlags = "allFlags";
        static inline std::string const kAllSegments = "allSegments";
        static inline std::string const kInitialized = "initialized";
    };
};
}  // namespace launchdarkly::server_side::data_systems
