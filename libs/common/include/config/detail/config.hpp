#pragma once

#include "config/detail/data_source_config.hpp"
#include "config/detail/endpoints_builder.hpp"
#include "config/detail/events_builder.hpp"
#include "config/detail/http_properties.hpp"
#include "logger.hpp"

namespace launchdarkly::config::detail {

/**
 * Config represents the configuration for a LaunchDarkly C++ SDK.
 * It should be passed into an instance of Client.
 * @tparam SDK Type of SDK.
 */
template <typename SDK>
struct Config {
   public:
    Config(std::string sdk_key,
           bool offline,
           launchdarkly::Logger logger,
           ServiceEndpoints endpoints,
           Events events,
           std::optional<std::string> application_tag,
           DataSourceConfig<SDK> data_source_config,
           detail::HttpProperties http_properties);

    [[nodiscard]] std::string const& SdkKey() const;

    [[nodiscard]] ServiceEndpoints const& ServiceEndpoints() const;

    [[nodiscard]] Events const& Events() const;

    [[nodiscard]] std::optional<std::string> const& ApplicationTag() const;

    DataSourceConfig<SDK> const& DataSourceConfig() const;

    [[nodiscard]] HttpProperties const& HttpProperties() const;

    [[nodiscard]] bool Offline() const;

    launchdarkly::Logger Logger();

   private:
    std::string sdk_key_;
    bool offline_;
    launchdarkly::Logger logger_;
    class ServiceEndpoints service_endpoints_;
    std::optional<std::string> application_tag_;
    class Events events_;
    ::launchdarkly::config::detail::DataSourceConfig<SDK> data_source_config_;
    detail::HttpProperties http_properties_;
};

}  // namespace launchdarkly::config::detail
