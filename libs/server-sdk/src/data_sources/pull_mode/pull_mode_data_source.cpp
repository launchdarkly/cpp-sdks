#include "pull_mode_data_source.hpp"

#include "../polling_data_source.hpp"
#include "../streaming_data_source.hpp"

namespace launchdarkly::server_side::data_sources {

using namespace config::shared::built;

PullModeSource::PullModeSource(
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

std::string PullModeSource::Identity() const {
    // TODO: Obtain more specific info
    return "generic pull-mode source";
}

std::weak_ptr<ISynchronizer> PullModeSource::GetSynchronizer() const {
    return synchronizer_;
}

std::weak_ptr<IBootstrapper> PullModeSource::GetBootstrapper() const {
    return bootstrapper_;
}

std::shared_ptr<FlagDescriptor> PullModeSource::GetFlag(
    std::string const& key) const {
    return store_.GetFlag(key);
}

std::shared_ptr<SegmentDescriptor> PullModeSource::GetSegment(
    std::string const& key) const {
    return store_.GetSegment(key);
}

std::unordered_map<std::string, std::shared_ptr<FlagDescriptor>>
PullModeSource::AllFlags() const {
    return store_.AllFlags();
}

std::unordered_map<std::string, std::shared_ptr<SegmentDescriptor>>
PullModeSource::AllSegments() const {
    return store_.AllSegments();
}

}  // namespace launchdarkly::server_side::data_sources
