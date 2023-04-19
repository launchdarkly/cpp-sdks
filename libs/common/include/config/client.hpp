#pragma once

#include "config/detail/builders/app_info_builder.hpp"
#include "config/detail/builders/config_builder.hpp"
#include "config/detail/builders/endpoints_builder.hpp"
#include "config/detail/builders/events_builder.hpp"
#include "config/detail/defaults.hpp"
#include "config/detail/sdks.hpp"

namespace launchdarkly::client_side {

using SDK = config::detail::ClientSDK;

using Defaults = config::detail::Defaults<SDK>;
using AppInfoBuilder = config::detail::builders::AppInfoBuilder;
using EndpointsBuilder = config::detail::builders::EndpointsBuilder<SDK>;
using ConfigBuilder = config::detail::builders::ConfigBuilder<SDK>;
using EventsBuilder = config::detail::builders::EventsBuilder<SDK>;
using DataSourceBuilder = config::detail::builders::DataSourceBuilder<SDK>;
using Config = config::detail::Config<SDK>;

}  // namespace launchdarkly::client_side
