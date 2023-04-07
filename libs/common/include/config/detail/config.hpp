#pragma once

#include "config/detail/endpoints_builder.hpp"
#include "config/detail/events_builder.hpp"

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
    detail::EventsBuilder<SDK> events_builder;
    Config(std::string sdk_key,
           bool offline,
           detail::EndpointsBuilder<SDK> service_endpoints_builder,
           detail::EventsBuilder<SDK> events_builder,
           std::optional<std::string> application_tag);
};

}  // namespace launchdarkly::config::detail
