#pragma once

#include "config/detail/config_builder.hpp"
#include "config/detail/endpoints_builder.hpp"
#include "config/detail/sdks.hpp"

namespace launchdarkly::client {

using SDK = config::detail::ClientSDK;

using EndpointsBuilder = config::detail::EndpointsBuilder<SDK>;
using ConfigBuilder = config::detail::ConfigBuilder<SDK>;
using Config = config::detail::Config<SDK>;

}  // namespace launchdarkly::client
