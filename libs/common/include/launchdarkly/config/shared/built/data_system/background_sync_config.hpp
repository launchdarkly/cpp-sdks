#pragma once

#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/built/data_system/bootstrap_config.hpp>
#include <launchdarkly/config/shared/built/data_system/data_destination_config.hpp>
#include <launchdarkly/config/shared/sdks.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>

namespace launchdarkly::config::shared::built {

template <typename SDK>
struct BackgroundSyncConfig {};

template <>
struct BackgroundSyncConfig<ClientSDK> {};

template <>
struct BackgroundSyncConfig<ServerSDK> {
    std::optional<BootstrapConfig> primary_bootstrapper_;
    std::optional<BootstrapConfig> fallback_bootstrapper_;
    DataSourceConfig<ServerSDK> source_;
    std::optional<DataDestinationConfig<ServerSDK>> destination_;
};

}  // namespace launchdarkly::config::shared::built
