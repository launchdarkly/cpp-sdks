#pragma once

#include "config/detail/built/data_source_config.hpp"
#include "config/detail/built/events.hpp"
#include "config/detail/built/http_properties.hpp"
#include "config/detail/built/service_endpoints.hpp"
#include "config/detail/sdks.hpp"

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

    static auto StreamingConfig() -> built::StreamingConfig {
        return {std::chrono::milliseconds{1000}};
    }

    static auto PollingConfig() -> built::PollingConfig {
        // Default to 5 minutes;
        return {std::chrono::seconds{5 * 60}};
    }
};

template <>
struct Defaults<ClientSDK> {
    static std::string SdkName() { return "CppClient"; }
    static std::string SdkVersion() { return "TODO"; }
    static bool Offline() { return Defaults<AnySDK>::Offline(); }

    static auto ServiceEndpoints() -> built::ServiceEndpoints {
        return {"https://clientsdk.launchdarkly.com",
                "https://clientstream.launchdarkly.com",
                "https://mobile.launchdarkly.com"};
    }

    static auto Events() -> built::Events {
        return {100,   std::chrono::seconds(30),      "/mobile",
                false, AttributeReference::SetType(), std::chrono::seconds(1)};
    }

    static auto HttpProperties() -> built::HttpProperties {
        return {
            std::chrono::milliseconds{10000}, std::chrono::milliseconds{10000},
            std::chrono::milliseconds{10000}, SdkName() + "/" + SdkVersion(),
            std::map<std::string, std::string>()};
    }

    static auto DataSourceConfig() -> built::DataSourceConfig<ClientSDK> {
        return {Defaults<AnySDK>::StreamingConfig(), false, false};
    }
};

template <>
struct Defaults<ServerSDK> {
    static std::string SdkName() { return "CppServer"; }
    static std::string SdkVersion() { return "TODO"; }
    static bool Offline() { return Defaults<AnySDK>::Offline(); }

    static auto ServiceEndpoints() -> built::ServiceEndpoints {
        return {"https://sdk.launchdarkly.com",
                "https://stream.launchdarkly.com",
                "https://events.launchdarkly.com"};
    }

    static auto Events() -> built::Events {
        return {10000, std::chrono::seconds(5),       "/bulk",
                false, AttributeReference::SetType(), std::chrono::seconds(1)};
    }

    static auto HttpProperties() -> built::HttpProperties {
        return {
            std::chrono::milliseconds{2000}, std::chrono::milliseconds{10000},
            std::chrono::milliseconds{10000}, SdkName() + "/" + SdkVersion(),
            std::map<std::string, std::string>()};
    }

    static auto DataSourceConfig() -> built::DataSourceConfig<ServerSDK> {
        return {Defaults<AnySDK>::StreamingConfig()};
    }
};

}  // namespace launchdarkly::config::detail
