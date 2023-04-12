#pragma once

#include "config/detail/data_source_config.hpp"
#include "config/detail/endpoints_builder.hpp"
#include "config/detail/http_properties.hpp"

namespace launchdarkly::config::detail {

/**
 * Config represents the configuration for a LaunchDarkly C++ SDK.
 * It should be passed into an instance of Client.
 * @tparam SDK Type of SDK.
 */
template <typename SDK>
struct Config {
    std::string sdk_key;
    bool offline;
    detail::EndpointsBuilder<SDK> service_endpoints_builder;
    std::optional<std::string> application_tag;
    DataSourceConfig<SDK> data_source_config;
    detail::HttpProperties http_properties;
    Config(std::string sdk_key,
           bool offline,
           detail::EndpointsBuilder<SDK> service_endpoints_builder,
           std::optional<std::string> application_tag,
           DataSourceConfig<SDK> data_source_config,
           detail::HttpProperties http_properties);
};

}  // namespace launchdarkly::config::detail
