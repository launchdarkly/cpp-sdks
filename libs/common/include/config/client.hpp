#pragma once

#include "config/detail/application_info.hpp"
#include "config/detail/config_builder.hpp"
#include "config/detail/hosts_builder.hpp"
#include "config/detail/sdks.hpp"

namespace launchdarkly::client {

using SDK = config::detail::ClientSDK;

using ApplicationInfo = config::detail::ApplicationInfo;
using Endpoints = config::detail::HostsBuilder<SDK>;
using ConfigBuilder = config::detail::ConfigBuilder<SDK>;
using EventsBuilder = config::detail::EventsBuilder<SDK>;
using Config = config::detail::Config<SDK>;

}  // namespace launchdarkly::client
