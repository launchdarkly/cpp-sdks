#pragma once

#include <launchdarkly/server_side/config/builders/all_builders.hpp>
#include <launchdarkly/server_side/config/config.hpp>

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
    config::builders::EndpointsBuilder& ServiceEndpoints();

    /**
     * To include metadata about the application that is utilizing the SDK,
     * pass in an AppInfoBuilder.
     * @param builder An AppInfoBuilder.
     * @return Reference to an AppInfoBuilder.
     */
    config::builders::AppInfoBuilder& AppInfo();

    /**
     * To tune settings related to event generation and delivery, pass an
     * EventsBuilder.
     * @param builder An EventsBuilder.
     * @return Reference to an EventsBuilder.
     */
    config::builders::EventsBuilder& Events();

    /**
     * Sets the configuration of the component that receives and stores feature
     * flag data from LaunchDarkly.
     * @param builder A DataSystemBuilder.
     * @return Reference to a DataSystemBuilder.
     */
    config::builders::DataSystemBuilder& DataSystem();

    /**
     * Sets the SDK's networking configuration, using an HttpPropertiesBuilder.
     * The builder has methods for setting individual HTTP-related properties.
     * @param builder A HttpPropertiesBuilder builder.
     * @return Reference to an HttpPropertiesBuilder.
     */
    config::builders::HttpPropertiesBuilder& HttpProperties();

    /**
     * Sets the logging configuration for the SDK.
     * @param builder A Logging builder.
     * @return Reference to a LoggingBuilder.
     */
    config::builders::LoggingBuilder& Logging();

    /**
     * @brief Equivalent to setting Events().Disable() and
     * DataSystem().Disable(). The effect is that all evaluations will return
     * application-provided default values, and no network calls will be made.
     * @return Reference to this.
     */
    ConfigBuilder& Offline();

    /**
     * Builds a Configuration, suitable for passing into an instance of Client.
     * @return
     */
    tl::expected<Result, Error> Build() const;

   private:
    std::string sdk_key_;
    std::optional<bool> offline_;

    config::builders::EndpointsBuilder service_endpoints_builder_;
    config::builders::AppInfoBuilder app_info_builder_;
    config::builders::EventsBuilder events_builder_;
    config::builders::DataSystemBuilder data_system_builder_;
    config::builders::HttpPropertiesBuilder http_properties_builder_;
    config::builders::LoggingBuilder logging_config_builder_;
};
}  // namespace launchdarkly::server_side
