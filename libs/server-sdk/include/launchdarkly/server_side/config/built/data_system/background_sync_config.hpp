#pragma once

#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/sdks.hpp>
#include <launchdarkly/server_side/config/built/data_system/bootstrap_config.hpp>
#include <launchdarkly/server_side/config/built/data_system/data_destination_config.hpp>

#include <optional>

namespace launchdarkly::server_side::config::built {

struct BackgroundSyncConfig {
    std::optional<BootstrapConfig> bootstrap_;
    launchdarkly::config::shared::built::DataSourceConfig<
        launchdarkly::config::shared::ServerSDK>
        source_;
    std::optional<DataDestinationConfig> destination_;
};

}  // namespace launchdarkly::server_side::config::built
