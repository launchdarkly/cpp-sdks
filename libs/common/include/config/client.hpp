#pragma once

#include "config/detail/config_builder.hpp"
#include "config/detail/sdks.hpp"

namespace launchdarkly::client {

using ConfigBuilder = config::detail::ConfigBuilder<config::detail::ClientSDK>;
using Config = config::detail::Config<config::detail::ClientSDK>;

}  // namespace launchdarkly::client
