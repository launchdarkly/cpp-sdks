#pragma once

#include <launchdarkly/config/shared/builders/app_info_builder.hpp>
#include <launchdarkly/config/shared/builders/data_source_builder.hpp>
#include <launchdarkly/config/shared/builders/data_system/data_systems_builder.hpp>
#include <launchdarkly/config/shared/builders/endpoints_builder.hpp>
#include <launchdarkly/config/shared/builders/events_builder.hpp>
#include <launchdarkly/config/shared/builders/http_properties_builder.hpp>
#include <launchdarkly/config/shared/builders/logging_builder.hpp>
#include <launchdarkly/config/shared/builders/persistence_builder.hpp>
#include <launchdarkly/config/shared/config.hpp>

#include <optional>
#include <string>

#include "tl/expected.hpp"

namespace launchdarkly::config::shared::builders {

template <typename SDK>
class ConfigBuilder {};

/**
 * ConfigBuilder allows for creation of a Configuration object for use
 * in a Client.
 * @tparam SDK Type of SDK.
 */
template <>
class ConfigBuilder<ClientSDK> {
   public:
    using SDK = ClientSDK;
    using Result = Config<SDK>;
    using EndpointsBuilder =
        launchdarkly::config::shared::builders::EndpointsBuilder<SDK>;
    using EventsBuilder =
        launchdarkly::config::shared::builders::EventsBuilder<SDK>;
    using DataSourceBuilder =
        launchdarkly::config::shared::builders::DataSourceBuilder<SDK>;
    using HttpPropertiesBuilder =
        launchdarkly::config::shared::builders::HttpPropertiesBuilder<SDK>;
    using PersistenceBuilder =
        launchdarkly::config::shared::builders::PersistenceBuilder<SDK>;
    using LoggingBuilder =
        launchdarkly::config::shared::builders::LoggingBuilder;

    /**
     * A minimal configuration consists of a LaunchDarkly SDK Key.
     * @param sdk_key SDK Key.
     */
    explicit ConfigBuilder(std::string sdk_key);

    /**
     * To customize the ServiceEndpoints the SDK uses for streaming,
     * polling, and events, pass in an EndpointsBuilder.
     * @return Reference to an EndpointsBuilder.
     */
    EndpointsBuilder& ServiceEndpoints();

    /**
     * To include metadata about the application that is utilizing the SDK,
     * pass in an AppInfoBuilder.
     * @return Reference to an AppInfoBuilder.
     */
    AppInfoBuilder& AppInfo();

    /**
     * Enables or disables "Offline" mode. True means
     * Offline mode is enabled.
     * @param offline True if the SDK should operate in Offline mode.
     * @return Reference to this builder.
     */
    ConfigBuilder& Offline(bool offline);

    /**
     * To tune settings related to event generation and delivery, pass an
     * EventsBuilder.
     * @return Reference to an EventsBuilder.
     */
    EventsBuilder& Events();

    /**
     * Sets the configuration of the component that receives feature flag data
     * from LaunchDarkly. Client-side SDK only.
     * @return Reference to a DataSourceBuilder.
     */
    DataSourceBuilder& DataSource();

    /**
     * Sets the SDK's networking configuration, using an HttpPropertiesBuilder.
     * The builder has methods for setting individual HTTP-related properties.
     * @return Reference to an HttpPropertiesBuilder.
     */
    HttpPropertiesBuilder& HttpProperties();

    /**
     * Sets the logging configuration for the SDK.
     * @return Reference to a LoggingBuilder.
     */
    LoggingBuilder& Logging();

    /**
     * Sets the persistence configuration for the SDK.
     * @return Reference to a PersistenceBuilder.
     */
    PersistenceBuilder& Persistence();

    /**
     * Builds a Configuration, suitable for passing into an instance of Client.
     * @return
     */
    tl::expected<Result, Error> Build() const;

   private:
    std::string sdk_key_;
    std::optional<bool> offline_;

    EndpointsBuilder service_endpoints_builder_;
    AppInfoBuilder app_info_builder_;
    EventsBuilder events_builder_;
    DataSourceBuilder data_source_builder_;
    HttpPropertiesBuilder http_properties_builder_;
    LoggingBuilder logging_config_builder_;
    PersistenceBuilder persistence_builder_;
};

template <>
class ConfigBuilder<ServerSDK> {
   public:
    using SDK = ServerSDK;
    using Result = Config<SDK>;
    using EndpointsBuilder =
        launchdarkly::config::shared::builders::EndpointsBuilder<SDK>;
    using EventsBuilder =
        launchdarkly::config::shared::builders::EventsBuilder<SDK>;
    using DataSourceBuilder =
        launchdarkly::config::shared::builders::DataSourceBuilder<SDK>;
    using HttpPropertiesBuilder =
        launchdarkly::config::shared::builders::HttpPropertiesBuilder<SDK>;
    using PersistenceBuilder =
        launchdarkly::config::shared::builders::PersistenceBuilder<SDK>;
    using LoggingBuilder =
        launchdarkly::config::shared::builders::LoggingBuilder;
    using DataSystemBuilder =
        launchdarkly::config::shared::builders::DataSystemBuilder<SDK>;
    /**
     * A minimal configuration consists of a LaunchDarkly SDK Key.
     * @param sdk_key SDK Key.
     */
    explicit ConfigBuilder(std::string sdk_key);

    /**
     * Configures the data system for the SDK. Server-side SDK only.
     * @return Reference to a DataSystemBuilder.
     */
    DataSystemBuilder& DataSystem();

    /**
     * To customize the ServiceEndpoints the SDK uses for streaming,
     * polling, and events, pass in an EndpointsBuilder.
     * @return Reference to an EndpointsBuilder.
     */
    EndpointsBuilder& ServiceEndpoints();

    /**
     * To include metadata about the application that is utilizing the SDK,
     * pass in an AppInfoBuilder.
     * @return Reference to an AppInfoBuilder.
     */
    AppInfoBuilder& AppInfo();

    /**
     * Enables or disables "Offline" mode. True means
     * Offline mode is enabled.
     * @param offline True if the SDK should operate in Offline mode.
     * @return Reference to this builder.
     */
    ConfigBuilder& Offline(bool offline);

    /**
     * To tune settings related to event generation and delivery, pass an
     * EventsBuilder.
     * @return Reference to an EventsBuilder.
     */
    EventsBuilder& Events();

    /**
     * Sets the SDK's networking configuration, using an HttpPropertiesBuilder.
     * The builder has methods for setting individual HTTP-related properties.
     * @return Reference to an HttpPropertiesBuilder.
     */
    HttpPropertiesBuilder& HttpProperties();

    /**
     * Sets the logging configuration for the SDK.
     * @return Reference to a LoggingBuilder.
     */
    LoggingBuilder& Logging();

    /**
     * Builds a Configuration, suitable for passing into an instance of Client.
     * @return
     */
    tl::expected<Result, Error> Build() const;

   private:
    std::string sdk_key_;
    std::optional<bool> offline_;

    EndpointsBuilder service_endpoints_builder_;
    AppInfoBuilder app_info_builder_;
    EventsBuilder events_builder_;
    HttpPropertiesBuilder http_properties_builder_;
    LoggingBuilder logging_config_builder_;
    DataSystemBuilder data_system_builder_;
};

}  // namespace launchdarkly::config::shared::builders
