#pragma once

#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/sdks.hpp>
#include <launchdarkly/server_side/config/built/data_system/bootstrap_config.hpp>
#include <launchdarkly/server_side/config/built/data_system/data_destination_config.hpp>

#include <optional>
#include <variant>

namespace launchdarkly::server_side::config::built {


struct BackgroundSyncConfig {
    using StreamingConfig = launchdarkly::config::shared::built::StreamingConfig<launchdarkly::config::shared::ServerSDK>;
    using PollingConfig = launchdarkly::config::shared::built::PollingConfig<launchdarkly::config::shared::ServerSDK>;


    std::optional<BootstrapConfig> bootstrap_;
    std::variant<StreamingConfig, PollingConfig> synchronizer_;
    std::optional<DataDestinationConfig> destination_;
};

}  // namespace launchdarkly::server_side::config::built
