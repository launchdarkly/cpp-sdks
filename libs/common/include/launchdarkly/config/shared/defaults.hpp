#pragma once

#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/built/data_system/data_system_config.hpp>
#include <launchdarkly/config/shared/built/events.hpp>
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

    static auto DataSystemConfig()
        -> shared::built::DataSystemConfig<ClientSDK> {
        // No usage of DataSystem config yet until next major version.
        return {};
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

    static auto HttpProperties() -> built::HttpProperties {
        return {std::chrono::seconds{10}, std::chrono::seconds{10},
                std::chrono::seconds{10}, std::chrono::seconds{10},
                std::map<std::string, std::string>()};
    }

    static auto StreamingConfig() -> built::StreamingConfig<ServerSDK> {
        return {std::chrono::seconds{1}, "/all"};
    }

    static auto DataSourceConfig() -> built::DataSourceConfig<ServerSDK> {
        return {StreamingConfig()};
    }

    // No bootstrap phase yet in server-sdk; instead full
    // sync is done when polling/streaming source initializes.
    static auto PrimaryBootstrapConfig()
        -> std::optional<built::BootstrapConfig> {
        return std::nullopt;
    }

    // Data isn't mirrored anywhere by default.
    static auto DataDestinationConfig()
        -> std::optional<built::DataDestinationConfig<ServerSDK>> {
        return std::nullopt;
    }

    static auto DataSystemConfig()
        -> built::DataSystemConfig<ServerSDK> {
        return {shared::built::BackgroundSyncConfig<ServerSDK>{
            PrimaryBootstrapConfig(),
            std::nullopt,
            DataSourceConfig(),
            DataDestinationConfig(),
        }};
    }

    static auto PollingConfig() -> built::PollingConfig<ServerSDK> {
        return {std::chrono::seconds{30}, "/sdk/latest-all",
                std::chrono::seconds{30}};
    }

    static auto PersistenceConfig() -> built::Persistence<ServerSDK> {
        return {};
    }
};

}  // namespace launchdarkly::config::shared
