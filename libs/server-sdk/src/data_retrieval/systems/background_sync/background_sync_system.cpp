#include "background_sync_system.hpp"

#include "../polling_data_source.hpp"
#include "../streaming_data_source.hpp"

namespace launchdarkly::server_side::data_retrieval {

using namespace config::shared::built;

PushModeSource::PushModeSource(
    ServiceEndpoints const& endpoints,
    DataSourceConfig<config::shared::ServerSDK> const& data_source_config,
    HttpProperties http_properties,
    boost::asio::any_io_executor ioc,
    DataSourceStatusManager& status_manager,
    Logger const& logger)
    : store_(), synchronizer_(), bootstrapper_() {
    std::visit(
        [&](auto&& method_config) {
            using T = std::decay_t<decltype(method_config)>;
            if constexpr (std::is_same_v<
                              T, StreamingConfig<config::shared::ServerSDK>>) {
                synchronizer_ =
                    std::make_shared<data_sources::StreamingDataSource>(
                        endpoints, method_config, http_properties, ioc, store_,
                        status_manager, logger);

            } else if constexpr (std::is_same_v<
                                     T, PollingConfig<
                                            config::shared::ServerSDK>>) {
                synchronizer_ =
                    std::make_shared<data_sources::PollingDataSource>(
                        endpoints, method_config, http_properties, ioc, store_,
                        status_manager, logger);
            }
        },
        data_source_config.method);
}

std::string const& PushModeSource::Identity() const {
    // TODO: Obtain more specific info
    static std::string id = "generic push-mode source";
    return id;
}

ISynchronizer* PushModeSource::GetSynchronizer() {
    return synchronizer_.get();
}

IBootstrapper* PushModeSource::GetBootstrapper() {
    return bootstrapper_.get();
}

std::shared_ptr<FlagDescriptor> PushModeSource::GetFlag(
    std::string const& key) const {
    return store_.GetFlag(key);
}

std::shared_ptr<SegmentDescriptor> PushModeSource::GetSegment(
    std::string const& key) const {
    return store_.GetSegment(key);
}

std::unordered_map<std::string, std::shared_ptr<FlagDescriptor>>
PushModeSource::AllFlags() const {
    return store_.AllFlags();
}

std::unordered_map<std::string, std::shared_ptr<SegmentDescriptor>>
PushModeSource::AllSegments() const {
    return store_.AllSegments();
}

}  // namespace launchdarkly::server_side::data_retrieval
