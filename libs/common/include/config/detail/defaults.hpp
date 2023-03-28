#pragma once

#include "config/service_endpoints.hpp"

namespace launchdarkly::config::detail {

/**
 * Represents a client-side SDK configured for production services.
 */
struct ClientSDK {};
/**
 * Represents a server-side SDK configured for production services.
 */
struct ServerSDK {};

/**
 * Struct templated over an SDK type, which makes available SDK-specific
 * configuration.
 * @tparam SDK Type of SDK. See ClientSDK, ServerSDK.
 */
template <typename SDK>
struct Defaults {};

template <>
struct Defaults<ClientSDK> {
    static ServiceEndpoints endpoints() {
        return {"https://clientsdk.launchdarkly.com",
                "https://clientstream.launchdarkly.com",
                "https://mobile.launchdarkly.com"};
    }
};

template <>
struct Defaults<ServerSDK> {
    static ServiceEndpoints endpoints() {
        return {"https://sdk.launchdarkly.com",
                "https://stream.launchdarkly.com",
                "https://events.launchdarkly.com"};
    }
};

}  // namespace launchdarkly::config::detail
