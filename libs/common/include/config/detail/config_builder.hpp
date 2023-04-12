#pragma once

#include <optional>
#include <string>
#include "config/detail/application_info.hpp"
#include "config/detail/config.hpp"
#include "config/detail/data_source_builder.hpp"
#include "config/detail/endpoints_builder.hpp"
#include "logger.hpp"

namespace launchdarkly::config::detail {

/**
 * ConfigBuilder allows for creation of a Configuration object for use
 * in a Client.
 * @tparam SDK Type of SDK.
 */
template <typename SDK>
class ConfigBuilder {
   public:
    using EndpointsBuilder = detail::EndpointsBuilder<SDK>;
    using ConfigType = detail::Config<SDK>;
    using DataSourceBuilder = detail::DataSourceBuilder<SDK>;
    /**
     * A minimal configuration consists of a LaunchDarkly SDK Key.
     * @param sdk_key SDK Key.
     */
    ConfigBuilder(std::string sdk_key);

    /**
     * To customize the endpoints the SDK uses for streaming, polling, and
     * events, pass in an EndpointsBuilder.
     * @param builder An EndpointsBuilder.
     * @return Reference to this builder.
     */
    ConfigBuilder& service_endpoints(detail::EndpointsBuilder<SDK> builder);

    /**
     * To include metadata about the application that is utilizing the SDK,
     * pass in an ApplicationInfo builder.
     * @param builder An ApplicationInfo builder.
     * @return Reference to this builder.
     */
    ConfigBuilder& application_info(detail::ApplicationInfo builder);

    /**
     * To enable or disable "offline" mode, pass a boolean value. True means
     * offline mode is enabled.
     * @param offline True if the SDK should operate in offline mode.
     * @return Reference to this builder.
     */
    ConfigBuilder& offline(bool offline);

    /**
     * Sets the configuration of the component that receives feature flag data
     * from LaunchDarkly.
     * @param builder A DataSourceConfig builder.
     * @return Reference to this builder.
     */
    ConfigBuilder& data_source(detail::DataSourceBuilder<SDK> builder);

    /**
     * Builds a Configuration, suitable for passing into an instance of Client.
     * @return
     */
    ConfigType build(Logger& logger) const;

   private:
    std::string sdk_key_;
    std::optional<bool> offline_;
    std::optional<EndpointsBuilder> service_endpoints_builder_;
    std::optional<ApplicationInfo> application_info_builder_;
    std::optional<DataSourceBuilder> data_source_builder_;
};

}  // namespace launchdarkly::config::detail
