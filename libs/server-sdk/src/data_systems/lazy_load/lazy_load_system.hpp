#pragma once

#include "../../data_components/expiration_tracker/expiration_tracker.hpp"
#include "../../data_components/kinds/kinds.hpp"
#include "../../data_components/memory_store/memory_store.hpp"
#include "../../data_interfaces/source/idata_reader.hpp"
#include "../../data_interfaces/system/idata_system.hpp"

#include <launchdarkly/server_side/config/built/data_system/lazy_load_config.hpp>
#include <launchdarkly/server_side/data_interfaces/sources/iserialized_data_reader.hpp>
#include <launchdarkly/server_side/integrations/serialized_item_descriptor.hpp>

#include <launchdarkly/data_model/descriptors.hpp>
#include <launchdarkly/detail/unreachable.hpp>
#include <launchdarkly/logging/logger.hpp>

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
class LazyLoad final : public data_interfaces::IDataSystem {
   public:
    using ClockType = std::chrono::steady_clock;
    using TimeFn = std::function<std::chrono::time_point<ClockType>()>;

    explicit LazyLoad(Logger const& logger, config::built::LazyLoadConfig cfg);
    LazyLoad(Logger const& logger,
             config::built::LazyLoadConfig cfg,
             TimeFn time);

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

    void Shutdown() override;

    // Public for usage in tests.
    struct Kinds {
        static data_components::FlagKind const Flag;
        static data_components::SegmentKind const Segment;
    };

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

    template <typename TResult>
    static TResult Get(data_components::ExpirationTracker::TrackState state,
                       std::function<void(void)> const& refresh,
                       std::function<TResult(void)> const& get) {
        switch (state) {
            case data_components::ExpirationTracker::TrackState::kStale:
                [[fallthrough]];
            case data_components::ExpirationTracker::TrackState::kNotTracked:
                refresh();
                [[fallthrough]];
            case data_components::ExpirationTracker::TrackState::kFresh:
                return get();
        }
        detail::unreachable();
    }

    ClockType::time_point ExpiryTime() const;

    Logger const& logger_;

    mutable data_components::MemoryStore cache_;
    std::unique_ptr<data_interfaces::IDataReader> reader_;

    mutable data_components::ExpirationTracker tracker_;
    TimeFn time_;
    mutable std::optional<bool> initialized_;

    ClockType::duration fresh_duration_;

    struct Keys {
        static inline std::string const kAllFlags = "allFlags";
        static inline std::string const kInitialized = "initialized";
    };
};
}  // namespace launchdarkly::server_side::data_systems
