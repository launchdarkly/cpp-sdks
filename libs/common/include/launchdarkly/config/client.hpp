#pragma once

#include <launchdarkly/config/shared/builders/app_info_builder.hpp>
#include <launchdarkly/config/shared/builders/config_builder.hpp>
#include <launchdarkly/config/shared/builders/endpoints_builder.hpp>
#include <launchdarkly/config/shared/builders/events_builder.hpp>
#include <launchdarkly/config/shared/built/persistence.hpp>
#include <launchdarkly/config/shared/defaults.hpp>
#include <launchdarkly/config/shared/sdks.hpp>

namespace launchdarkly::client_side {

using SDK = config::shared::ClientSDK;

using Defaults = config::shared::Defaults<SDK>;
using AppInfoBuilder = config::shared::builders::AppInfoBuilder;
using EndpointsBuilder = config::shared::builders::EndpointsBuilder<SDK>;

using ConfigBuilder = config::shared::builders::ConfigBuilder<SDK>;

using EventsBuilder = config::shared::builders::EventsBuilder<SDK>;
using HttpPropertiesBuilder =
    config::shared::builders::HttpPropertiesBuilder<SDK>;
using DataSourceBuilder = config::shared::builders::DataSourceBuilder<SDK>;
using LoggingBuilder = config::shared::builders::LoggingBuilder;
using PersistenceBuilder = config::shared::builders::PersistenceBuilder<SDK>;

using Config = config::Config<SDK>;

}  // namespace launchdarkly::client_side
