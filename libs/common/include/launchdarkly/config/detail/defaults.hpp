#pragma once

#include <launchdarkly/config/detail/built/data_source_config.hpp>
#include <launchdarkly/config/detail/built/events.hpp>
#include <launchdarkly/config/detail/built/http_properties.hpp>
#include <launchdarkly/config/detail/built/service_endpoints.hpp>
#include <launchdarkly/config/detail/sdks.hpp>

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
        return {true,
                100,
                std::chrono::seconds(30),
                "/mobile",
                false,
                AttributeReference::SetType(),
                std::chrono::seconds(1),
                5};
    }

    static auto HttpProperties() -> built::HttpProperties {
        return {std::chrono::seconds{10},
                std::chrono::seconds{10},
                std::chrono::seconds{10},
                std::chrono::seconds{10},
                SdkName() + "/" + SdkVersion(),
                std::map<std::string, std::string>()};
    }

    static auto StreamingConfig() -> built::StreamingConfig<ClientSDK> {
        return {std::chrono::seconds{1}, "/meval"};
    }

    static auto DataSourceConfig() -> built::DataSourceConfig<ClientSDK> {
        return {Defaults<ClientSDK>::StreamingConfig(), false, false};
    }

    static auto PollingConfig() -> built::PollingConfig<ClientSDK> {
        // Default to 5 minutes;
        return {std::chrono::seconds{5 * 60}, "/msdk/evalx/contexts",
                "/msdk/evalx/context", std::chrono::seconds{30}};
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
        return {true,
                10000,
                std::chrono::seconds(5),
                "/bulk",
                false,
                AttributeReference::SetType(),
                std::chrono::seconds(1),
                5};
    }

    static auto HttpProperties() -> built::HttpProperties {
        return {std::chrono::seconds{2},
                std::chrono::seconds{10},
                std::chrono::seconds{10},
                std::chrono::seconds{10},
                SdkName() + "/" + SdkVersion(),
                std::map<std::string, std::string>()};
    }

    static auto StreamingConfig() -> built::StreamingConfig<ServerSDK> {
        return {std::chrono::seconds{1}};
    }

    static auto DataSourceConfig() -> built::DataSourceConfig<ServerSDK> {
        return {Defaults<ServerSDK>::StreamingConfig()};
    }

    static auto PollingConfig() -> built::PollingConfig<ServerSDK> {
        return {std::chrono::seconds{30}};
    }
};

}  // namespace launchdarkly::config::detail
