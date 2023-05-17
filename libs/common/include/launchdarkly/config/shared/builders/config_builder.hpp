#pragma once

#include <launchdarkly/config/shared/builders/app_info_builder.hpp>
#include <launchdarkly/config/shared/builders/data_source_builder.hpp>
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

/**
 * ConfigBuilder allows for creation of a Configuration object for use
 * in a Client.
 * @tparam SDK Type of SDK.
 */
template <typename SDK>
class ConfigBuilder {
   public:
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
    /**
     * A minimal configuration consists of a LaunchDarkly SDK Key.
     * @param sdk_key SDK Key.
     */
    explicit ConfigBuilder(std::string sdk_key);

    /**
     * To customize the ServiceEndpoints the SDK uses for streaming, polling,
     * and events, pass in an EndpointsBuilder.
     * @param builder An EndpointsBuilder.
     * @return Reference to this builder.
     */
    ConfigBuilder& ServiceEndpoints(EndpointsBuilder builder);

    /**
     * To include metadata about the application that is utilizing the SDK,
     * pass in an AppInfoBuilder.
     * @param builder An AppInfoBuilder.
     * @return Reference to this builder.
     */
    ConfigBuilder& AppInfo(AppInfoBuilder builder);

    /**
     * To enable or disable "Offline" mode, pass a boolean value. True means
     * Offline mode is enabled.
     * @param offline True if the SDK should operate in Offline mode.
     * @return Reference to this builder.
     */
    ConfigBuilder& Offline(bool offline);

    /**
     * To tune settings related to event generation and delivery, pass an
     * EventsBuilder.
     * @param builder An EventsBuilder.
     * @return Reference to this builder.
     */
    ConfigBuilder& Events(EventsBuilder builder);

    /**
     * Sets the configuration of the component that receives feature flag data
     * from LaunchDarkly.
     * @param builder A DataSourceConfig builder.
     * @return Reference to this builder.
     */
    ConfigBuilder& DataSource(DataSourceBuilder builder);

    /**
     * Sets the SDK's networking configuration, using an HttpPropertiesBuilder.
     * The builder has methods for setting individual HTTP-related properties.
     * @param builder A HttpPropertiesBuilder builder.
     * @return Reference to this builder.
     */
    ConfigBuilder& HttpProperties(HttpPropertiesBuilder builder);

    /**
     * Sets the logging configuration for the SDK.
     * @param builder A Logging builder.
     * @return Reference to this builder.
     */
    ConfigBuilder& Logging(LoggingBuilder builder);

    /**
     * Sets the persistence configuration for the SDK.
     * @param builder A persistence builder.
     * @return Reference to this builder.
     */
    ConfigBuilder& Persistence(PersistenceBuilder builder);

    /**
     * Builds a Configuration, suitable for passing into an instance of Client.
     * @return
     */
    tl::expected<Result, Error> Build() const;

   private:
    std::string sdk_key_;
    std::optional<bool> offline_;
    std::optional<EndpointsBuilder> service_endpoints_builder_;
    std::optional<AppInfoBuilder> app_info_builder_;
    std::optional<EventsBuilder> events_builder_;
    std::optional<DataSourceBuilder> data_source_builder_;
    std::optional<HttpPropertiesBuilder> http_properties_builder_;
    std::optional<LoggingBuilder> logging_config_builder_;
    std::optional<PersistenceBuilder> persistence_builder_;
};

}  // namespace launchdarkly::config::shared::builders
