#pragma once

#include "../../../include/launchdarkly/server_side/integrations/data_reader/kinds.hpp"
#include "../../data_components/expiration_tracker/expiration_tracker.hpp"
#include "../../data_components/memory_store/memory_store.hpp"
#include "../../data_components/status_notifications/data_source_status_manager.hpp"
#include "../../data_interfaces/source/idata_reader.hpp"
#include "../../data_interfaces/system/idata_system.hpp"

#include <launchdarkly/server_side/config/built/data_system/lazy_load_config.hpp>
#include <launchdarkly/server_side/integrations/data_reader/iserialized_data_reader.hpp>

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

    explicit LazyLoad(Logger const& logger,
                      config::built::LazyLoadConfig cfg,
                      data_components::DataSourceStatusManager& status_manager);

    LazyLoad(Logger const& logger,
             config::built::LazyLoadConfig cfg,
             data_components::DataSourceStatusManager& status_manager,
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

    void Initialize() override;

    bool Initialized() const override;

    // Public for usage in tests.
    struct Kinds {
        static integrations::FlagKind const Flag;
        static integrations::SegmentKind const Segment;
    };

   private:
    void RefreshAllFlags() const;
    void RefreshAllSegments() const;
    void RefreshInitState() const;
    void RefreshFlag(std::string const& key) const;
    void RefreshSegment(std::string const& key) const;

    static std::string CacheTraceMsg(
        data_components::ExpirationTracker::TrackState state);

    template <typename TResult>
    TResult Get(std::string const& key,
                data_components::ExpirationTracker::TrackState const state,

                std::function<void(void)> const& refresh,
                std::function<TResult(void)> const& get) const {
        LD_LOG(logger_, LogLevel::kDebug)
            << Identity() << ": get " << key << " - " << CacheTraceMsg(state);

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

    template <typename Item, typename Evictor>
    void RefreshItem(
        data_components::DataKind const kind,
        std::string const& key,
        std::function<data_interfaces::IDataReader::SingleResult<Item>(
            std::string const&)> const& getter,
        Evictor&& evictor) const {
        // Refreshing this item is always rate limited, even
        // if the refresh has an error.
        tracker_.Add(kind, key, ExpiryTime());

        if (auto expected_item = getter(key)) {
            status_manager_.SetState(DataSourceState::kValid);

            if (auto optional_item = *expected_item) {
                // This transformation is necessary because the memory store
                // works with ItemDescriptors, whereas the reader operates using
                // IDataReader::StorageItems. This doesn't necessarily need to
                // be the case.
                cache_.Upsert(
                    key,
                    data_interfaces::IDataReader::StorageItemIntoDescriptor(
                        std::move(*optional_item)));

            } else {
                // If the item is actually *missing* - not just a deleted
                // tombstone representation - it implies that the source
                // was re-initialized. In this case, the correct thing to do
                // is evict it from the memory cache
                LD_LOG(logger_, LogLevel::kDebug)
                    << kind << key << " requested but not found via "
                    << reader_->Identity();
                if (evictor(key)) {
                    LD_LOG(logger_, LogLevel::kDebug)
                        << "removed " << kind << " " << key << " from cache";
                }
            }
        } else {
            status_manager_.SetState(
                DataSourceState::kInterrupted,
                common::data_sources::DataSourceStatusErrorKind::kUnknown,
                expected_item.error());

            // If there's a persistent error, it will be logged at the refresh
            // interval.
            LD_LOG(logger_, LogLevel::kError)
                << "failed to refresh " << kind << " " << key << " via "
                << reader_->Identity() << ": " << expected_item.error();
        }
    }

    template <typename Item>
    void RefreshAll(
        std::string const& all_item_key,
        data_components::DataKind const item_kind,
        std::function<
            data_interfaces::IDataReader::CollectionResult<Item>()> const&
            getter) const {
        // Storing an expiry time so that the 'all' key and the individual
        // item keys will expire at the same time.
        auto const updated_expiry = ExpiryTime();

        // Refreshing 'all' for this item is always rate limited, even if
        // the refresh has an error.
        tracker_.Add(all_item_key, updated_expiry);

        if (auto all_items = getter()) {
            status_manager_.SetState(DataSourceState::kValid);

            for (auto item : *all_items) {
                // This transformation is necessary because the memory store
                // works with ItemDescriptors, whereas the reader operates using
                // IDataReader::StorageItems. This doesn't necessarily need to
                // be the case.
                cache_.Upsert(
                    item.first,
                    data_interfaces::IDataReader::StorageItemIntoDescriptor(
                        std::move(item.second)));
                tracker_.Add(item.first, updated_expiry);
            }
        } else {
            status_manager_.SetState(
                DataSourceState::kInterrupted,
                common::data_sources::DataSourceStatusErrorKind::kUnknown,
                all_items.error());

            // If there's a persistent error, it will be logged at the
            // refresh interval.
            LD_LOG(logger_, LogLevel::kError)
                << "failed to refresh all " << item_kind << "s via "
                << reader_->Identity() << ": " << all_items.error();
        }
    }

    ClockType::time_point ExpiryTime() const;

    Logger const& logger_;

    mutable data_components::MemoryStore cache_;
    std::unique_ptr<data_interfaces::IDataReader> reader_;

    data_components::DataSourceStatusManager& status_manager_;

    mutable data_components::ExpirationTracker tracker_;
    TimeFn time_;
    mutable std::optional<bool> initialized_;

    ClockType::duration fresh_duration_;

    struct Keys {
        static inline std::string const kAllFlags = "allFlags";
        static inline std::string const kAllSegments = "allSegments";
        static inline std::string const kInitialized = "initialized";
    };
};
}  // namespace launchdarkly::server_side::data_systems
