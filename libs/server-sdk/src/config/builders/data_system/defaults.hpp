#pragma once
#include <launchdarkly/server_side/config/built/data_system/background_sync_config.hpp>
#include <launchdarkly/server_side/config/built/data_system/data_destination_config.hpp>
#include <launchdarkly/server_side/config/built/data_system/data_system_config.hpp>
#include <launchdarkly/server_side/config/built/data_system/fdv2_config.hpp>
#include <launchdarkly/server_side/config/built/data_system/lazy_load_config.hpp>

namespace launchdarkly::server_side::config {

struct Defaults {
    // No bootstrap phase yet in server-sdk; instead full
    // sync is done when polling/streaming source initializes.
    static auto BootstrapConfig() -> std::optional<built::BootstrapConfig> {
        return std::nullopt;
    }

    // Data isn't mirrored anywhere by default.
    static auto DataDestinationConfig()
        -> std::optional<built::DataDestinationConfig> {
        return std::nullopt;
    }

    static auto SynchronizerConfig()
        -> built::BackgroundSyncConfig::StreamingConfig {
        return {std::chrono::seconds(1), "/all"};
    }

    static auto BackgroundSyncConfig() -> built::BackgroundSyncConfig {
        return {BootstrapConfig(), SynchronizerConfig(),
                DataDestinationConfig()};
    }

    static auto LazyLoadConfig() -> built::LazyLoadConfig {
        return {built::LazyLoadConfig::EvictionPolicy::Disabled,
                std::chrono::minutes{5}, nullptr};
    }

    static auto FDv2StreamingConfig() -> built::FDv2Config::StreamingConfig {
        return {std::chrono::seconds{1}, "/all"};
    }

    static auto FDv2PollingConfig() -> built::FDv2Config::PollingConfig {
        return {std::chrono::seconds{30}, "/sdk/latest-all",
                std::chrono::seconds{30}};
    }

    static auto FDv2Config() -> built::FDv2Config {
        using StreamingConfig = built::FDv2Config::StreamingConfig;
        using PollingConfig = built::FDv2Config::PollingConfig;
        using SyncVariant = std::variant<StreamingConfig, PollingConfig>;
        return {{FDv2PollingConfig()},
                std::vector<SyncVariant>{FDv2StreamingConfig(),
                                         FDv2PollingConfig()},
                SyncVariant{FDv2StreamingConfig()},
                std::chrono::minutes{2},
                std::chrono::minutes{5}};
    }

    static auto DataSystemConfig() -> built::DataSystemConfig {
        return {false, BackgroundSyncConfig()};
    }
};
}  // namespace launchdarkly::server_side::config
