#pragma once

#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/built/events.hpp>
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/config/shared/sdks.hpp>
#include <launchdarkly/logging/log_level.hpp>

namespace launchdarkly::config::shared {

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

    static std::string LogTag() { return "LaunchDarkly"; }
    static launchdarkly::LogLevel LogLevel() { return LogLevel::kInfo; }
};

template <>
struct Defaults<ClientSDK> {
    static bool Offline() { return Defaults<AnySDK>::Offline(); }

    static auto ServiceEndpoints() -> shared::built::ServiceEndpoints {
        return {"https://clientsdk.launchdarkly.com",
                "https://clientstream.launchdarkly.com",
                "https://mobile.launchdarkly.com"};
    }

    static auto Events() -> shared::built::Events {
        return {true,
                100,
                std::chrono::seconds(30),
                "/mobile",
                false,
                AttributeReference::SetType(),
                std::chrono::seconds(1),
                5};
    }

    static auto HttpProperties() -> shared::built::HttpProperties {
        return {std::chrono::seconds{10}, std::chrono::seconds{10},
                std::chrono::seconds{10}, std::chrono::seconds{10},
                std::map<std::string, std::string>()};
    }

    static auto StreamingConfig() -> shared::built::StreamingConfig<ClientSDK> {
        return {std::chrono::seconds{1}, "/meval"};
    }

    static auto DataSourceConfig()
        -> shared::built::DataSourceConfig<ClientSDK> {
        return {Defaults<ClientSDK>::StreamingConfig(), false, false};
    }

    static auto PollingConfig() -> shared::built::PollingConfig<ClientSDK> {
        return {std::chrono::minutes(5), "/msdk/evalx/contexts",
                "/msdk/evalx/context", std::chrono::minutes(5)};
    }

    static std::size_t MaxCachedContexts() { return 5; }
};

template <>
struct Defaults<ServerSDK> {
    static bool Offline() { return Defaults<AnySDK>::Offline(); }

    static auto ServiceEndpoints() -> shared::built::ServiceEndpoints {
        return {"https://sdk.launchdarkly.com",
                "https://stream.launchdarkly.com",
                "https://events.launchdarkly.com"};
    }

    static auto Events() -> shared::built::Events {
        return {true,
                10000,
                std::chrono::seconds(5),
                "/bulk",
                false,
                AttributeReference::SetType(),
                std::chrono::seconds(1),
                5};
    }

    static auto HttpProperties() -> shared::built::HttpProperties {
        return {std::chrono::seconds{2}, std::chrono::seconds{10},
                std::chrono::seconds{10}, std::chrono::seconds{10},
                std::map<std::string, std::string>()};
    }

    static auto StreamingConfig() -> shared::built::StreamingConfig<ServerSDK> {
        return {std::chrono::seconds{1}};
    }

    static auto DataSourceConfig()
        -> shared::built::DataSourceConfig<ServerSDK> {
        return {Defaults<ServerSDK>::StreamingConfig()};
    }

    static auto PollingConfig() -> shared::built::PollingConfig<ServerSDK> {
        return {std::chrono::seconds{30}};
    }
};

}  // namespace launchdarkly::config::shared
