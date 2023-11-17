#pragma once

#include <launchdarkly/server_side/config/builders/data_system/bootstrap_builder.hpp>
#include <launchdarkly/server_side/config/builders/data_system/data_destination_builder.hpp>
#include <launchdarkly/server_side/config/built/data_system/background_sync_config.hpp>

#include <launchdarkly/config/shared/builders/data_source_builder.hpp>

namespace launchdarkly::server_side::config::builders {

struct BackgroundSyncBuilder {
    using Streaming =
        launchdarkly::config::shared::builders::StreamingBuilder<launchdarkly::config::shared::ServerSDK>;
    using Polling = launchdarkly::config::shared::builders::PollingBuilder<launchdarkly::config::shared::ServerSDK>;

    BackgroundSyncBuilder();

    BootstrapBuilder& Bootstrapper();

    BackgroundSyncBuilder& Synchronizer(Streaming source);
    BackgroundSyncBuilder& Synchronizer(Polling source);

    BackgroundSyncBuilder& Destination(DataDestinationBuilder destination);

    [[nodiscard]] built::BackgroundSyncConfig Build() const;

   private:
    BootstrapBuilder bootstrap_builder_;
    built::BackgroundSyncConfig config_;
};

}  // namespace launchdarkly::server_side::config::builders
