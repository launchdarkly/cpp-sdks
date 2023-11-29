#include "background_sync_system.hpp"

#include "sources/noop/null_data_source.hpp"
#include "sources/polling/polling_data_source.hpp"
#include "sources/streaming/streaming_data_source.hpp"

namespace launchdarkly::server_side::data_systems {

BackgroundSync::BackgroundSync(
    config::built::ServiceEndpoints const& endpoints,
    config::built::BackgroundSyncConfig const& background_sync_config,
    config::built::HttpProperties http_properties,
    boost::asio::any_io_executor ioc,
    data_components::DataSourceStatusManager& status_manager,
    Logger const& logger)
    : store_(), change_notifier_(store_, store_), synchronizer_() {
    std::visit(
        [&](auto&& method_config) {
            using T = std::decay_t<decltype(method_config)>;
            if constexpr (std::is_same_v<T,
                                         config::built::BackgroundSyncConfig::
                                             StreamingConfig>) {
                synchronizer_ = std::make_shared<StreamingDataSource>(
                    ioc, logger, status_manager, endpoints, method_config,
                    http_properties);

            } else if constexpr (std::is_same_v<
                                     T, config::built::BackgroundSyncConfig::
                                            PollingConfig>) {
                synchronizer_ = std::make_shared<PollingDataSource>(
                    ioc, logger, status_manager, endpoints, method_config,
                    http_properties);
            }
        },
        background_sync_config.synchronizer_);
}

void BackgroundSync::Initialize() {
    synchronizer_->StartAsync(&change_notifier_,
                              nullptr /* no bootstrap data supported yet */);
}

bool BackgroundSync::Initialized() const {
    return store_.Initialized();
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
