#pragma once

#include <launchdarkly/config/shared/builders/app_info_builder.hpp>
#include <launchdarkly/config/shared/builders/config_builder.hpp>
#include <launchdarkly/config/shared/builders/data_system/data_systems_builder.hpp>
#include <launchdarkly/config/shared/builders/endpoints_builder.hpp>
#include <launchdarkly/config/shared/builders/events_builder.hpp>
#include <launchdarkly/config/shared/defaults.hpp>
#include <launchdarkly/config/shared/sdks.hpp>

namespace launchdarkly::server_side {

using SDK = config::shared::ServerSDK;

using Defaults = config::shared::Defaults<SDK>;
using AppInfoBuilder = config::shared::builders::AppInfoBuilder;
using EndpointsBuilder = config::shared::builders::EndpointsBuilder<SDK>;
using EventsBuilder = config::shared::builders::EventsBuilder<SDK>;
using HttpPropertiesBuilder =
    config::shared::builders::HttpPropertiesBuilder<SDK>;
using DataSystemBuilder = config::shared::builders::DataSystemBuilder<SDK>;
using LoggingBuilder = config::shared::builders::LoggingBuilder;

}  // namespace launchdarkly::server_side
