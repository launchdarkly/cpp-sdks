#pragma once

#include <boost/variant.hpp>
#include <chrono>
#include <optional>
#include <type_traits>
#include "config/detail/sdks.hpp"

namespace launchdarkly::config::detail::built {

template <typename SDK>
struct StreamingConfig;

template <>
struct StreamingConfig<ClientSDK> {
    std::chrono::milliseconds initial_reconnect_delay;

    inline static const std::string streaming_path = "/meval";
};

template <>
struct StreamingConfig<ServerSDK> {
    std::chrono::milliseconds initial_reconnect_delay;
};

template <typename SDK>
struct PollingConfig;

template <>
struct PollingConfig<ClientSDK> {
    std::chrono::seconds poll_interval;

    inline const static std::string polling_get_path = "/msdk/evalx/contexts";

    inline const static std::string polling_report_path = "/msdk/evalx/context";

    inline const static std::chrono::seconds min_polling_interval =
        std::chrono::seconds{30};
};

template <>
struct PollingConfig<ServerSDK> {
    std::chrono::seconds poll_interval;
};

template <typename SDK>
struct DataSourceConfig;

template <>
struct DataSourceConfig<ClientSDK> {
    boost::variant<StreamingConfig<ClientSDK>, PollingConfig<ClientSDK>> method;

    bool with_reasons;
    bool use_report;
};

template <>
struct DataSourceConfig<ServerSDK> {
    boost::variant<StreamingConfig<ServerSDK>, PollingConfig<ServerSDK>> method;
};

}  // namespace launchdarkly::config::detail::built
