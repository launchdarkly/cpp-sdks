#pragma once

#include <launchdarkly/config/shared/builders/app_info_builder.hpp>
#include <launchdarkly/config/shared/builders/endpoints_builder.hpp>
#include <launchdarkly/config/shared/builders/events_builder.hpp>
#include <launchdarkly/config/shared/builders/http_properties_builder.hpp>
#include <launchdarkly/config/shared/builders/logging_builder.hpp>

#include <launchdarkly/server_side/config/builders/data_system/background_sync_builder.hpp>
#include <launchdarkly/server_side/config/builders/data_system/bootstrap_builder.hpp>
#include <launchdarkly/server_side/config/builders/data_system/data_destination_builder.hpp>
#include <launchdarkly/server_side/config/builders/data_system/data_system_builder.hpp>
#include <launchdarkly/server_side/config/builders/data_system/lazy_load_builder.hpp>

namespace launchdarkly::server_side::config::builders {

using SDK = launchdarkly::config::shared::ServerSDK;
using EndpointsBuilder =
    launchdarkly::config::shared::builders::EndpointsBuilder<SDK>;
using HttpPropertiesBuilder =
    launchdarkly::config::shared::builders::HttpPropertiesBuilder<SDK>;
using AppInfoBuilder = launchdarkly::config::shared::builders::AppInfoBuilder;
using EventsBuilder =
    launchdarkly::config::shared::builders::EventsBuilder<SDK>;
using LoggingBuilder = launchdarkly::config::shared::builders::LoggingBuilder;
using TlsBuilder = launchdarkly::config::shared::builders::TlsBuilder<SDK>;

}  // namespace launchdarkly::server_side::config::builders
