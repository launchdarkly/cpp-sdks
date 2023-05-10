#pragma once

#include <optional>
#include <string>
#include "config/detail/builders/app_info_builder.hpp"
#include "config/detail/builders/data_source_builder.hpp"
#include "config/detail/builders/endpoints_builder.hpp"
#include "config/detail/builders/events_builder.hpp"
#include "config/detail/builders/http_properties_builder.hpp"
#include "config/detail/config.hpp"
#include "tl/expected.hpp"

#include "logger.hpp"

namespace launchdarkly::config::detail::builders {

/**
 * ConfigBuilder allows for creation of a Configuration object for use
 * in a Client.
 * @tparam SDK Type of SDK.
 */
template <typename SDK>
class ConfigBuilder {
   public:
    using Result = detail::Config<SDK>;
    using EndpointsBuilder = detail::builders::EndpointsBuilder<SDK>;
    using EventsBuilder = detail::builders::EventsBuilder<SDK>;
    using DataSourceBuilder = detail::builders::DataSourceBuilder<SDK>;
    using HttpPropertiesBuilder = detail::builders::HttpPropertiesBuilder<SDK>;
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
};

}  // namespace launchdarkly::config::detail::builders
