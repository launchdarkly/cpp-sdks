#pragma once

#include <launchdarkly/config/shared/built/events.hpp>
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <launchdarkly/config/shared/built/logging.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>

#include <launchdarkly/server_side/config/built/data_system/bootstrap_config.hpp>
#include <launchdarkly/server_side/config/built/data_system/data_destination_config.hpp>
#include <launchdarkly/server_side/config/built/data_system/data_system_config.hpp>
#include <launchdarkly/server_side/config/built/data_system/lazy_load_config.hpp>
#include <launchdarkly/server_side/config/built/data_system/background_sync_config.hpp>

namespace launchdarkly::server_side::config::built {

using Events = launchdarkly::config::shared::built::Events;
using HttpProperties = launchdarkly::config::shared::built::HttpProperties;
using Logging = launchdarkly::config::shared::built::Logging;
using ServiceEndpoints = launchdarkly::config::shared::built::ServiceEndpoints;

}  // namespace launchdarkly::server_side::config::built
