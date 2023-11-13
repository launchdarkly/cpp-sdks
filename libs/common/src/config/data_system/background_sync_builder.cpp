#include <launchdarkly/config/shared/builders/data_system/background_sync_builder.hpp>

namespace launchdarkly::config::shared::builders {

BackgroundSyncBuilder<ServerSDK>::BackgroundSyncBuilder()
    : bootstrap_builder_(), config_() {}

BootstrapBuilder& BackgroundSyncBuilder<ServerSDK>::Bootstrapper() {
    return bootstrap_builder_;
}

BackgroundSyncBuilder<ServerSDK>&
BackgroundSyncBuilder<ServerSDK>::Synchronizer(Streaming source) {
    config_.source_.method = source.Build();
    return *this;
}

BackgroundSyncBuilder<ServerSDK>&
BackgroundSyncBuilder<ServerSDK>::Synchronizer(Polling source) {
    config_.source_.method = source.Build();
    return *this;
}

BackgroundSyncBuilder<ServerSDK>& BackgroundSyncBuilder<ServerSDK>::Destination(
    DataDestinationBuilder destination) {
    config_.destination_ = destination.Build();
    return *this;
}

[[nodiscard]] built::BackgroundSyncConfig<ServerSDK>
BackgroundSyncBuilder<ServerSDK>::Build() const {
    auto const bootstrap_cfg = bootstrap_builder_.Build();
    auto copy = config_;
    copy.bootstrap_ = bootstrap_cfg;
    return copy;
}

}  // namespace launchdarkly::config::shared::builders
