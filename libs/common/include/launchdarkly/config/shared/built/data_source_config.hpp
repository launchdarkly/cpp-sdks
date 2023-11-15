#pragma once

#include <launchdarkly/config/shared/sdks.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>

namespace launchdarkly::config::shared::built {

template <typename SDK>
struct StreamingConfig;

template <>
struct StreamingConfig<ClientSDK> {
    std::chrono::milliseconds initial_reconnect_delay;
    std::string streaming_path;
};

template <>
struct StreamingConfig<ServerSDK> {
    std::chrono::milliseconds initial_reconnect_delay;
    std::string streaming_path;
};

template <typename SDK>
struct PollingConfig;

template <>
struct PollingConfig<ClientSDK> {
    std::chrono::seconds poll_interval;
    std::string polling_get_path;
    std::string polling_report_path;
    std::chrono::seconds min_polling_interval;
};

template <>
struct PollingConfig<ServerSDK> {
    std::chrono::seconds poll_interval;
    std::string polling_get_path;
    std::chrono::seconds min_polling_interval;
};

struct RedisPullConfig {
    using URI = std::string;

    struct ConnectionOpts {
        std::string host;
        std::uint16_t port;
        std::string password;
        std::uint64_t db;
    };

    std::variant<URI, ConnectionOpts> connection_;
};

template <typename SDK>
struct DataSourceConfig;

template <>
struct DataSourceConfig<ClientSDK> {
    std::variant<StreamingConfig<ClientSDK>, PollingConfig<ClientSDK>> method;

    bool with_reasons;
    bool use_report;
};

template <>
struct DataSourceConfig<ServerSDK> {
    std::variant<StreamingConfig<ServerSDK>,
                 PollingConfig<ServerSDK>,
                 RedisPullConfig>
        method;
};

}  // namespace launchdarkly::config::shared::built
