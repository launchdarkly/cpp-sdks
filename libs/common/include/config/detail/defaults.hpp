#pragma once

#include "config/detail/events.hpp"
#include "config/detail/http_properties.hpp"
#include "config/detail/sdks.hpp"
#include "config/detail/service_endpoints.hpp"

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
    static std::string sdk_name() { return "CppClient"; }
    static std::string sdk_version() { return "TODO"; }
    static ServiceEndpoints endpoints() {
        return {"https://clientsdk.launchdarkly.com",
                "https://clientstream.launchdarkly.com",
                "https://mobile.launchdarkly.com"};
    }

    static Events events() {
        return {100, std::chrono::seconds(30), "/mobile", false,
                AttributeReference::SetType()};
    }

    static HttpProperties http_properties() {
        return {std::chrono::milliseconds{10000},
                std::chrono::milliseconds{10000},
                sdk_name() + "/" + sdk_version(),
                std::map<std::string, std::string>()};
    }
};

template <>
struct Defaults<ServerSDK> {
    static std::string sdk_name() { return "CppServer"; }
    static std::string sdk_version() { return "TODO"; }
    static ServiceEndpoints endpoints() {
        return {"https://sdk.launchdarkly.com",
                "https://stream.launchdarkly.com",
                "https://events.launchdarkly.com"};
    }

    static Events events() {
        return {10000, std::chrono::seconds(5), "/bulk", false,
                AttributeReference::SetType()};
    }

    static HttpProperties http_properties() {
        return {std::chrono::milliseconds{2000},
                std::chrono::milliseconds{10000},
                sdk_name() + "/" + sdk_version(),
                std::map<std::string, std::string>()};
    }
};

}  // namespace launchdarkly::config::detail
