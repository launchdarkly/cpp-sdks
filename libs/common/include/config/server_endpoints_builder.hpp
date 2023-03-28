#pragma once

#include "config/detail/defaults.hpp"
#include "config/detail/endpoints_builder.hpp"

namespace launchdarkly::config {

/**
 * ServiceEndpoints builder for the Server-side SDK.
 */
using ServerEndpointsBuilder = detail::EndpointsBuilder<detail::ServerSDK>;

}  // namespace launchdarkly::config
