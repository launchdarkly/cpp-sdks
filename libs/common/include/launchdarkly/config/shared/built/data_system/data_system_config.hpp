#pragma once

#include <launchdarkly/config/shared/built/data_system/background_sync_config.hpp>
#include <launchdarkly/config/shared/sdks.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>

namespace launchdarkly::config::shared::built {

template <typename SDK>
struct DataSystemConfig;

template <>
struct DataSystemConfig<ClientSDK> {};

template <>
struct DataSystemConfig<ServerSDK> {
    bool disabled;
    std::variant<
        /*LazyLoadConfig<ServerSDK>, */ BackgroundSyncConfig<ServerSDK>>
        system_;
};

}  // namespace launchdarkly::config::shared::built
