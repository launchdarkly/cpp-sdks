#pragma once

#include "config/detail/defaults.hpp"
#include "config/detail/endpoints_builder.hpp"

namespace launchdarkly::config {

/**
 * ServiceEndpoints builder for the Client-side SDK.
 */
using ClientEndpointsBuilder = detail::EndpointsBuilder<detail::ClientSDK>;

}  // namespace launchdarkly::config
