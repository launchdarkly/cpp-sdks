#pragma once

#include "config/detail/defaults.hpp"
#include "config/endpoints_builder.hpp"

namespace launchdarkly::config {

using ClientEndpointsBuilder = EndpointsBuilder<detail::ClientSDK>;

}  // namespace launchdarkly::config
