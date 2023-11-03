#pragma once
#include <launchdarkly/config/server_builders.hpp>
#include <launchdarkly/server_side/config/config.hpp>
#include "launchdarkly/config/shared/builders/app_info_builder.hpp"
#include "launchdarkly/config/shared/builders/data_source_builder.hpp"
#include "launchdarkly/config/shared/builders/http_properties_builder.hpp"
#include "launchdarkly/config/shared/builders/logging_builder.hpp"
#include "launchdarkly/config/shared/defaults.hpp"

namespace launchdarkly::server_side {

class ConfigBuilder {
   public:
    using Result = Config;
    /**
     * A minimal configuration consists of a LaunchDarkly SDK Key.
     * @param sdk_key SDK Key.
     */
    explicit ConfigBuilder(std::string sdk_key);

    /**
     * To customize the ServiceEndpoints the SDK uses for streaming,
     * polling, and events, pass in an EndpointsBuilder.
     * @param builder An EndpointsBuilder.
     * @return Reference to an EndpointsBuilder.
     */
    EndpointsBuilder& ServiceEndpoints();

    /**
     * To include metadata about the application that is utilizing the SDK,
     * pass in an AppInfoBuilder.
     * @param builder An AppInfoBuilder.
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
     * @param builder An EventsBuilder.
     * @return Reference to an EventsBuilder.
     */
    EventsBuilder& Events();

    /**
     * Sets the configuration of the component that receives feature flag data
     * from LaunchDarkly.
     * @param builder A DataSourceConfig builder.
     * @return Reference to a DataSourceBuilder.
     */
    DataSourceBuilder& DataSource();

    /**
     * Sets the SDK's networking configuration, using an HttpPropertiesBuilder.
     * The builder has methods for setting individual HTTP-related properties.
     * @param builder A HttpPropertiesBuilder builder.
     * @return Reference to an HttpPropertiesBuilder.
     */
    HttpPropertiesBuilder& HttpProperties();

    /**
     * Sets the logging configuration for the SDK.
     * @param builder A Logging builder.
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
    DataSourceBuilder data_source_builder_;
    HttpPropertiesBuilder http_properties_builder_;
    LoggingBuilder logging_config_builder_;
};
}  // namespace launchdarkly::server_side
