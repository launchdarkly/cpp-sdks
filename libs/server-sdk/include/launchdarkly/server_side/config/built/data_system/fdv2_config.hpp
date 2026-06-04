#pragma once

#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/sdks.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace launchdarkly::server_side::config::built {

struct FDv2Config {
    struct StreamingConfig {
        std::chrono::milliseconds initial_reconnect_delay;
        std::optional<std::string> filter_key;
        std::optional<std::string> base_url_override;

        friend bool operator==(StreamingConfig const& lhs,
                               StreamingConfig const& rhs) {
            return lhs.initial_reconnect_delay == rhs.initial_reconnect_delay &&
                   lhs.filter_key == rhs.filter_key &&
                   lhs.base_url_override == rhs.base_url_override;
        }
    };

    struct PollingConfig {
        std::chrono::seconds poll_interval;
        std::optional<std::string> filter_key;
        std::optional<std::string> base_url_override;

        friend bool operator==(PollingConfig const& lhs,
                               PollingConfig const& rhs) {
            return lhs.poll_interval == rhs.poll_interval &&
                   lhs.filter_key == rhs.filter_key &&
                   lhs.base_url_override == rhs.base_url_override;
        }
    };

    using FDv1StreamingConfig =
        launchdarkly::config::shared::built::StreamingConfig<
            launchdarkly::config::shared::ServerSDK>;
    using FDv1PollingConfig =
        launchdarkly::config::shared::built::PollingConfig<
            launchdarkly::config::shared::ServerSDK>;

    std::vector<PollingConfig> initializers;
    std::vector<std::variant<StreamingConfig, PollingConfig>> synchronizers;
    std::optional<std::variant<FDv1StreamingConfig, FDv1PollingConfig>>
        fdv1_fallback;
    std::chrono::milliseconds fallback_timeout;
    std::chrono::milliseconds recovery_timeout;
};

}  // namespace launchdarkly::server_side::config::built
