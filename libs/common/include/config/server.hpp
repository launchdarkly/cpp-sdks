#pragma once

#include "config/detail/application_info.hpp"
#include "config/detail/config_builder.hpp"
#include "config/detail/hosts_builder.hpp"
#include "config/detail/sdks.hpp"

namespace launchdarkly::server {

using SDK = config::detail::ServerSDK;

using ApplicationInfo = config::detail::ApplicationInfo;
using HostsBuilder = config::detail::HostsBuilder<SDK>;
using ConfigBuilder = config::detail::ConfigBuilder<SDK>;
using EventsBuilder = config::detail::EventsBuilder<SDK>;
using Config = config::detail::Config<SDK>;

}  // namespace launchdarkly::server
