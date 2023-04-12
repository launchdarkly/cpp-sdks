#pragma once

#include <boost/variant.hpp>
#include <chrono>
#include <optional>
#include <type_traits>
#include "config/detail/sdks.hpp"

namespace launchdarkly::config::detail {

struct StreamingConfig {
    std::chrono::milliseconds initial_reconnect_delay;
};

struct PollingConfig {
    std::chrono::seconds poll_interval;
};

template <typename SDK>
struct DataSourceConfig;

template <>
struct DataSourceConfig<ClientSDK> {
    boost::variant<StreamingConfig, PollingConfig> method;

    bool with_reasons;
    bool use_report;
};

template <>
struct DataSourceConfig<ServerSDK> {
    boost::variant<StreamingConfig, PollingConfig> method;
};

}  // namespace launchdarkly::config::detail
