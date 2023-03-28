#pragma once

#include "config/detail/defaults.hpp"
#include "config/endpoints_builder.hpp"

namespace launchdarkly::config {

using ServerEndpointsBuilder = EndpointsBuilder<detail::ServerSDK>;

}  // namespace launchdarkly::config
