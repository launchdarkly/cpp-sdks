#pragma once

#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/built/data_system/bootstrap_config.hpp>
#include <launchdarkly/config/shared/built/data_system/data_destination_config.hpp>
#include <launchdarkly/config/shared/sdks.hpp>

#include <optional>

namespace launchdarkly::config::shared::built {

template <typename SDK>
struct BackgroundSyncConfig {};

template <>
struct BackgroundSyncConfig<ClientSDK> {};

template <>
struct BackgroundSyncConfig<ServerSDK> {
    std::optional<BootstrapConfig> bootstrap_;
    DataSourceConfig<ServerSDK> source_;
    std::optional<DataDestinationConfig<ServerSDK>> destination_;
};

}  // namespace launchdarkly::config::shared::built
