#pragma once

#include "config/service_endpoints.hpp"

namespace launchdarkly::config::detail {

struct ClientSDK {};
struct ServerSDK {};

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
