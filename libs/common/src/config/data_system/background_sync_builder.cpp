#include <launchdarkly/config/shared/builders/data_system/background_sync_builder.hpp>

namespace launchdarkly::config::shared::builders {

BackgroundSyncBuilder<ServerSDK>::BackgroundSyncBuilder() : config_() {}

BackgroundSyncBuilder<ServerSDK>& BackgroundSyncBuilder<
    ServerSDK>::PrimaryBootstrapper(BootstrapBuilder bootstrap) {
    config_.primary_bootstrapper_ = bootstrap.Build();
    return *this;
}

BackgroundSyncBuilder<ServerSDK>& BackgroundSyncBuilder<
    ServerSDK>::FallbackBootstrapper(BootstrapBuilder bootstrap) {
    config_.fallback_bootstrapper_ = bootstrap.Build();
    return *this;
}

BackgroundSyncBuilder<ServerSDK>& BackgroundSyncBuilder<ServerSDK>::Source(
    Streaming source) {
    config_.source_.method = source.Build();
    return *this;
}

BackgroundSyncBuilder<ServerSDK>& BackgroundSyncBuilder<ServerSDK>::Source(
    Polling source) {
    config_.source_.method = source.Build();
    return *this;
}

BackgroundSyncBuilder<ServerSDK>& BackgroundSyncBuilder<ServerSDK>::Destination(
    DataDestinationBuilder destination) {
    config_.destination_ = destination.Build();
    return *this;
}

[[nodiscard]] config::shared::built::BackgroundSyncConfig<ServerSDK>
BackgroundSyncBuilder<ServerSDK>::Build() const {
    return config_;
}

}  // namespace launchdarkly::config::shared::builders
