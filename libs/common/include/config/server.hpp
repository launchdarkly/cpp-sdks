#pragma once

#include "config/detail/config_builder.hpp"
#include "config/detail/sdks.hpp"

namespace launchdarkly::server {

using ConfigBuilder = config::detail::ConfigBuilder<config::detail::ServerSDK>;
using Config = config::detail::Config<config::detail::ServerSDK>;

}  // namespace launchdarkly::server
