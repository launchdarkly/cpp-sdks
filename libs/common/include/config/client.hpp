#pragma once

#include "config/detail/app_info_builder.hpp"
#include "config/detail/config_builder.hpp"
#include "config/detail/endpoints_builder.hpp"
#include "config/detail/sdks.hpp"

namespace launchdarkly::client {

using SDK = config::detail::ClientSDK;

using AppInfoBuilder = config::detail::AppInfoBuilder;
using EndpointsBuilder = config::detail::EndpointsBuilder<SDK>;
using ConfigBuilder = config::detail::ConfigBuilder<SDK>;
using EventsBuilder = config::detail::EventsBuilder<SDK>;
using Config = config::detail::Config<SDK>;

}  // namespace launchdarkly::client
