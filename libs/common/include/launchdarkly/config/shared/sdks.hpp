#pragma once

namespace launchdarkly::config::shared {
/**
 * Represents a client-side SDK configured for production services.
 */
struct ClientSDK {};
/**
 * Represents a server-side SDK configured for production services.
 */
struct ServerSDK {};

/**
 * Represents configuration not common to any particular SDK type.
 */
struct AnySDK {};

}  // namespace launchdarkly::config
