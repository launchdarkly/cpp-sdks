#include "background_sync_system.hpp"

#include "sources/noop/null_data_source.hpp"
#include "sources/polling/polling_data_source.hpp"
#include "sources/streaming/streaming_data_source.hpp"

namespace launchdarkly::server_side::data_systems {

using namespace config::shared::built;

BackgroundSync::BackgroundSync(
    ServiceEndpoints const& endpoints,
    BackgroundSyncConfig<config::shared::ServerSDK> const&
        background_sync_config,
    HttpProperties http_properties,
    boost::asio::any_io_executor ioc,
    data_components::DataSourceStatusManager& status_manager,
    Logger const& logger)
    : store_(), change_notifier_(store_, store_), synchronizer_() {
    std::visit(
        [&](auto&& method_config) {
            using T = std::decay_t<decltype(method_config)>;
            if constexpr (std::is_same_v<
                              T, StreamingConfig<config::shared::ServerSDK>>) {
                synchronizer_ = std::make_shared<StreamingDataSource>(
                    endpoints, method_config, http_properties, ioc, store_,
                    status_manager, logger);

            } else if constexpr (std::is_same_v<
                                     T, PollingConfig<
                                            config::shared::ServerSDK>>) {
                synchronizer_ = std::make_shared<PollingDataSource>(
                    endpoints, method_config, http_properties, ioc, store_,
                    status_manager, logger);
            }
        },
        background_sync_config.source_.method);
}

BackgroundSync::BackgroundSync(
    boost::asio::any_io_executor ioc,
    data_components::DataSourceStatusManager& status_manager)
    : store_(),
      change_notifier_(store_, store_),
      synchronizer_(std::make_shared<NullDataSource>(ioc, status_manager)) {}

void BackgroundSync::Initialize() {
    // TODO: if there was any data from bootstrapping, then add it:
    // synchronizer_->Init(data);
    synchronizer_->Start();
}

std::string const& BackgroundSync::Identity() const {
    static std::string id = "background sync via " + synchronizer_->Identity();
    return id;
}

std::shared_ptr<data_model::FlagDescriptor> BackgroundSync::GetFlag(
    std::string const& key) const {
    return store_.GetFlag(key);
}

std::shared_ptr<data_model::SegmentDescriptor> BackgroundSync::GetSegment(
    std::string const& key) const {
    return store_.GetSegment(key);
}

std::unordered_map<std::string, std::shared_ptr<data_model::FlagDescriptor>>
BackgroundSync::AllFlags() const {
    return store_.AllFlags();
}

std::unordered_map<std::string, std::shared_ptr<data_model::SegmentDescriptor>>
BackgroundSync::AllSegments() const {
    return store_.AllSegments();
}

}  // namespace launchdarkly::server_side::data_systems
