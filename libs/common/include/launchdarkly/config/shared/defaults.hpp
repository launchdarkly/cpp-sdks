#pragma once

#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/built/events.hpp>
#include <launchdarkly/config/shared/built/fdv2_config.hpp>
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <launchdarkly/config/shared/built/persistence.hpp>
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
                5,
                std::nullopt};
    }

    static auto TLS() -> shared::built::TlsOptions { return {}; }

    static auto HttpProperties() -> shared::built::HttpProperties {
        return {std::chrono::seconds{10},
                std::chrono::seconds{10},
                std::chrono::seconds{10},
                std::chrono::seconds{10},
                std::map<std::string, std::string>(),
                TLS(),
                shared::built::ProxyOptions()};
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

    /**
     * The three connection modes a desktop SDK provides, starting in
     * streaming, with automatic mode switching off.
     */
    static auto FDv2Config() -> shared::built::FDv2Config<ClientSDK> {
        using Config = shared::built::FDv2Config<ClientSDK>;

        // Both timeouts are chosen for consistency with the other
        // LaunchDarkly SDKs.
        auto const fallback_timeout = std::chrono::seconds(120);
        auto const recovery_timeout = std::chrono::seconds(300);

        Config::StreamingConfig const streaming{std::chrono::seconds(1),
                                                std::nullopt};
        Config::PollingConfig const polling{std::chrono::minutes(5),
                                            std::nullopt};
        Config::FDv1FallbackConfig const fdv1_fallback{std::chrono::minutes(5),
                                                       std::nullopt};

        return {
            shared::ConnectionMode::kStreaming,
            "https://sdk.launchdarkly.com",
            "https://clientstream.launchdarkly.com",
            {
                // Streaming initializes from the cache, then polls for a
                // basis so that the stream can deliver only what changed,
                // and falls back to polling if the stream cannot be kept up.
                {shared::ConnectionMode::kStreaming,
                 Config::ModeDefinition{{Config::CacheConfig{}, polling},
                                        {streaming, polling},
                                        fdv1_fallback}},
                {shared::ConnectionMode::kPolling,
                 Config::ModeDefinition{
                     {Config::CacheConfig{}}, {polling}, fdv1_fallback}},
                // Offline evaluates against the cache and makes no requests,
                // so it has nothing to fall back to.
                {shared::ConnectionMode::kOffline,
                 Config::ModeDefinition{
                     {Config::CacheConfig{}}, {}, std::nullopt}},
            },
            /* use_post= */ false,
            fallback_timeout,
            recovery_timeout,
        };
    }

    static std::size_t MaxCachedContexts() { return 5; }
};

template <>
struct Defaults<ServerSDK> {
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
                5,
                1000};
    }

    static auto TLS() -> shared::built::TlsOptions { return {}; }

    static auto HttpProperties() -> built::HttpProperties {
        return {std::chrono::seconds{10},
                std::chrono::seconds{10},
                std::chrono::seconds{10},
                std::chrono::seconds{10},
                std::map<std::string, std::string>(),
                TLS(),
                built::ProxyOptions()};
    }

    static auto StreamingConfig() -> built::StreamingConfig<ServerSDK> {
        return {std::chrono::seconds{1}, "/all"};
    }

    static auto PollingConfig() -> built::PollingConfig<ServerSDK> {
        return {std::chrono::seconds{30}, "/sdk/latest-all",
                std::chrono::seconds{30}};
    }
};

}  // namespace launchdarkly::config::shared
