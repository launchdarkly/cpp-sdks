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

    std::string const& sdk_key() const;

    ServiceEndpoints const& service_endpoints() const;

    Events const& events_config() const;

    std::optional<std::string> const& application_tag() const;

    DataSourceConfig<SDK> const& data_source_config() const;

    HttpProperties const& http_properties() const;

    bool offline() const;

    Logger take_logger();

   private:
    std::string sdk_key_;
    bool offline_;
    launchdarkly::Logger logger_;
    ServiceEndpoints service_endpoints_;
    std::optional<std::string> application_tag_;
    Events events_;
    DataSourceConfig<SDK> data_source_config_;
    detail::HttpProperties http_properties_;
};

}  // namespace launchdarkly::config::detail
