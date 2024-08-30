#include <launchdarkly/server_side/config/builders/data_system/background_sync_builder.hpp>

#include "defaults.hpp"

namespace launchdarkly::server_side::config::builders {
BackgroundSyncBuilder::BackgroundSyncBuilder()
    : bootstrap_builder_(), config_(Defaults::BackgroundSyncConfig()) {
}

BootstrapBuilder& BackgroundSyncBuilder::Bootstrapper() {
    return bootstrap_builder_;
}

BackgroundSyncBuilder& BackgroundSyncBuilder::Synchronizer(
    Streaming source) {
    if (auto cfg = source.Build(); !cfg) {
        synchronizer_cfg_ = tl::make_unexpected(cfg.error());
    } else {
        synchronizer_cfg_ = *cfg;
    }
    return *this;
}

BackgroundSyncBuilder& BackgroundSyncBuilder::Synchronizer(
    Polling source) {
    if (auto cfg = source.Build(); !cfg) {
        synchronizer_cfg_ = tl::make_unexpected(cfg.error());
    } else {
        synchronizer_cfg_ = *cfg;
    }
    return *this;
}

BackgroundSyncBuilder& BackgroundSyncBuilder::Destination(
    DataDestinationBuilder destination) {
    config_.destination_ = destination.Build();
    return *this;
}

[[nodiscard]] tl::expected<built::BackgroundSyncConfig, Error>
BackgroundSyncBuilder::Build() const {
    auto const bootstrap_cfg = bootstrap_builder_.Build();

    if (!synchronizer_cfg_) {
        return tl::make_unexpected(synchronizer_cfg_.error());
    }

    auto copy = config_;
    copy.bootstrap_ = bootstrap_cfg;
    copy.synchronizer_ = *synchronizer_cfg_;
    return copy;
}
} // namespace launchdarkly::server_side::config::builders
