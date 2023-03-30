#pragma once

#include "config/detail/application_info.hpp"
#include "config/detail/config_builder.hpp"
#include "config/detail/endpoints_builder.hpp"
#include "config/detail/sdks.hpp"

namespace launchdarkly::server {

using SDK = config::detail::ServerSDK;

using ApplicationInfo = config::detail::ApplicationInfo;
using Endpoints = config::detail::EndpointsBuilder<SDK>;
using ConfigBuilder = config::detail::ConfigBuilder<SDK>;
using Config = config::detail::Config<SDK>;

}  // namespace launchdarkly::server
