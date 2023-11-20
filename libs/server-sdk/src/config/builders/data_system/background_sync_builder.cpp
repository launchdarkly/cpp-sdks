#include <launchdarkly/server_side/config/builders/data_system/background_sync_builder.hpp>

namespace launchdarkly::server_side::config::builders {

BackgroundSyncBuilder::BackgroundSyncBuilder()
    : bootstrap_builder_(), config_() {}

BootstrapBuilder& BackgroundSyncBuilder::Bootstrapper() {
    return bootstrap_builder_;
}

BackgroundSyncBuilder& BackgroundSyncBuilder::Synchronizer(Streaming source) {
    config_.source_.method = source.Build();
    return *this;
}

BackgroundSyncBuilder& BackgroundSyncBuilder::Synchronizer(Polling source) {
    config_.source_.method = source.Build();
    return *this;
}

BackgroundSyncBuilder& BackgroundSyncBuilder::Destination(
    DataDestinationBuilder destination) {
    config_.destination_ = destination.Build();
    return *this;
}

[[nodiscard]] built::BackgroundSyncConfig BackgroundSyncBuilder::Build() const {
    auto const bootstrap_cfg = bootstrap_builder_.Build();
    auto copy = config_;
    copy.bootstrap_ = bootstrap_cfg;
    return copy;
}

}  // namespace launchdarkly::server_side::config::builders
