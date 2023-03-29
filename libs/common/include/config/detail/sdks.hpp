#pragma once

namespace launchdarkly::config::detail {
/**
 * Represents a client-side SDK configured for production services.
 */
struct ClientSDK {};
/**
 * Represents a server-side SDK configured for production services.
 */
struct ServerSDK {};

}  // namespace launchdarkly::config::detail
