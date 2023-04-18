#pragma once

#include "config/detail/data_source_config.hpp"
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
    static bool Offline() { return false; }

    static StreamingConfig StreamingConfig() {
        return {std::chrono::milliseconds{1000}};
    }

    static PollingConfig PollingConfig() {
        // Default to 5 minutes;
        return {std::chrono::seconds{5 * 60}};
    }
};

template <>
struct Defaults<ClientSDK> {
    static std::string SdkName() { return "CppClient"; }
    static std::string SdkVersion() { return "TODO"; }
    static ServiceEndpoints Endpoints() {
        return {"https://clientsdk.launchdarkly.com",
                "https://clientstream.launchdarkly.com",
                "https://mobile.launchdarkly.com"};
    }

    static Events Events() {
        return {100, std::chrono::seconds(30), "/mobile", false,
                AttributeReference::SetType()};
    }

    static HttpProperties HttpProperties() {
        return {std::chrono::milliseconds{10000},
                std::chrono::milliseconds{10000},
                SdkName() + "/" + SdkVersion(),
                std::map<std::string, std::string>()};
    }

    static DataSourceConfig<ClientSDK> DataSource() {
        return {Defaults<AnySDK>::StreamingConfig(), false, false};
    }

    static bool offline() { return Defaults<AnySDK>::Offline(); }
};

template <>
struct Defaults<ServerSDK> {
    static std::string SdkName() { return "CppServer"; }
    static std::string SdkVersion() { return "TODO"; }
    static ServiceEndpoints Endpoints() {
        return {"https://sdk.launchdarkly.com",
                "https://stream.launchdarkly.com",
                "https://events.launchdarkly.com"};
    }

    static Events Events() {
        return {10000, std::chrono::seconds(5), "/bulk", false,
                AttributeReference::SetType()};
    }

    static HttpProperties HttpProperties() {
        return {std::chrono::milliseconds{2000},
                std::chrono::milliseconds{10000},
                SdkName() + "/" + SdkVersion(),
                std::map<std::string, std::string>()};
    }

    static DataSourceConfig<ServerSDK> DataSource() {
        return {Defaults<AnySDK>::StreamingConfig()};
    }

    static bool offline() { return Defaults<AnySDK>::Offline(); }
};

}  // namespace launchdarkly::config::detail
