#pragma once

#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/sdks.hpp>

#include <chrono>
#include <optional>
#include <variant>
#include <vector>

namespace launchdarkly::server_side::config::built {

struct FDv2Config {
    using StreamingConfig =
        launchdarkly::config::shared::built::StreamingConfig<
            launchdarkly::config::shared::ServerSDK>;
    using PollingConfig = launchdarkly::config::shared::built::PollingConfig<
        launchdarkly::config::shared::ServerSDK>;

    std::vector<PollingConfig> initializers;
    std::vector<std::variant<StreamingConfig, PollingConfig>> synchronizers;
    std::optional<std::variant<StreamingConfig, PollingConfig>> fdv1_fallback;
    std::chrono::milliseconds fallback_timeout;
    std::chrono::milliseconds recovery_timeout;
};

}  // namespace launchdarkly::server_side::config::built
