#pragma once

#include "../../data_components/change_notifier/change_notifier_destination.hpp"
#include "../../data_components/memory_store/memory_store.hpp"
#include "../../data_components/status_notifications/data_source_status_manager.hpp"
#include "../../data_interfaces/source/ipush_source.hpp"
#include "../../data_interfaces/system/isystem.hpp"

#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/built/data_system/background_sync_config.hpp>
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/data_model/descriptors.hpp>
#include <launchdarkly/logging/logger.hpp>

#include <boost/asio/any_io_executor.hpp>

namespace launchdarkly::server_side::data_systems {

/**
 * BackgroundSync implements the standard Data System which receives
 * data updates from LaunchDarkly as they happen. It updates an in-memory
 * store with the data, and always operates with a complete dataset.
 *
 * The BackgroundSync system is advantageous because it allows flag evaluations
 * to take place with up-to-date information. However, some environments may be
 * too large to fit in memory, or a direct connection to LaunchDarkly isn't
 * desired, necessitating use of the alternate LazyLoad system.
 */
class BackgroundSync : public data_interfaces::ISystem {
   public:
    BackgroundSync(config::shared::built::ServiceEndpoints const& endpoints,
                   config::shared::built::BackgroundSyncConfig<
                       config::shared::ServerSDK> const& background_sync_config,
                   config::shared::built::HttpProperties http_properties,
                   boost::asio::any_io_executor ioc,
                   data_components::DataSourceStatusManager& status_manager,
                   Logger const& logger);

    /**
     * @brief Constructs a BackgroundSync without a data source.
     */
    BackgroundSync(boost::asio::any_io_executor ioc,
                   data_components::DataSourceStatusManager& status_manager);

    BackgroundSync(BackgroundSync const& item) = delete;
    BackgroundSync(BackgroundSync&& item) = delete;
    BackgroundSync& operator=(BackgroundSync const&) = delete;
    BackgroundSync& operator=(BackgroundSync&&) = delete;

    std::shared_ptr<data_model::FlagDescriptor> GetFlag(
        std::string const& key) const override;
    std::shared_ptr<data_model::SegmentDescriptor> GetSegment(
        std::string const& key) const override;
    std::unordered_map<std::string, std::shared_ptr<data_model::FlagDescriptor>>
    AllFlags() const override;
    std::unordered_map<std::string,
                       std::shared_ptr<data_model::SegmentDescriptor>>
    AllSegments() const override;

    std::string const& Identity() const override;

    void Initialize() override;

   private:
    data_components::MemoryStore store_;
    data_components::ChangeNotifierDestination change_notifier_;
    // Needs to be shared to that the source can keep itself alive through
    // async operations.
    std::shared_ptr<data_interfaces::IPushSource> synchronizer_;
};
}  // namespace launchdarkly::server_side::data_systems