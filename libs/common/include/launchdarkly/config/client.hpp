#pragma once

#include <launchdarkly/config/shared/builders/app_info_builder.hpp>
#include <launchdarkly/config/shared/builders/config_builder.hpp>
#include <launchdarkly/config/shared/builders/endpoints_builder.hpp>
#include <launchdarkly/config/shared/builders/events_builder.hpp>
#include <launchdarkly/config/shared/defaults.hpp>
#include <launchdarkly/config/shared/sdks.hpp>

namespace launchdarkly::client_side {

using SDK = config::detail::ClientSDK;

using Defaults = config::detail::Defaults<SDK>;
using AppInfoBuilder = config::detail::builders::AppInfoBuilder;
using EndpointsBuilder = config::detail::builders::EndpointsBuilder<SDK>;
using ConfigBuilder = config::detail::builders::ConfigBuilder<SDK>;
using EventsBuilder = config::detail::builders::EventsBuilder<SDK>;
using HttpPropertiesBuilder =
    config::detail::builders::HttpPropertiesBuilder<SDK>;
using DataSourceBuilder = config::detail::builders::DataSourceBuilder<SDK>;

using Config = config::detail::Config<SDK>;

}  // namespace launchdarkly::client_side
