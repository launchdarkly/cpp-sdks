#pragma once

#include "config/detail/events.hpp"
#include "config/detail/sdks.hpp"
#include "config/detail/service_hosts.hpp"

namespace launchdarkly::config::detail {

/**
 * Struct templated over an SDK type, which makes available SDK-specific
 * configuration.
 * @tparam SDK Type of SDK. See ClientSDK, ServerSDK.
 */
template <typename SDK>
struct Defaults {
    /**
     * Offline mode is disabled in SDKs by default.
     * @return
     */
    static bool offline() { return false; }
};

template <>
struct Defaults<ClientSDK> {
    static ServiceHosts endpoints() {
        return {"clientsdk.launchdarkly.com", "clientstream.launchdarkly.com",
                "mobile.launchdarkly.com"};
    }
    static Events events() {
        return {
            100,
            std::chrono::seconds(30),
            "/mobile",
            AttributePolicy::Default,
            TransportSecurity::TLS,
        };
    }
};

template <>
struct Defaults<ServerSDK> {
    static ServiceHosts endpoints() {
        return {"sdk.launchdarkly.com", "stream.launchdarkly.com",
                "events.launchdarkly.com"};
    }
    static Events events() {
        return {
            10000,
            std::chrono::seconds(5),
            "/bulk",
            AttributePolicy::Default,
            TransportSecurity::TLS,
        };
    }
};

}  // namespace launchdarkly::config::detail
