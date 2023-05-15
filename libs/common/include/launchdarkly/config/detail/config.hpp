#pragma once

#include <launchdarkly/config/detail/builders/endpoints_builder.hpp>
#include <launchdarkly/config/detail/builders/events_builder.hpp>
#include <launchdarkly/config/detail/built/data_source_config.hpp>
#include <launchdarkly/config/detail/built/events.hpp>
#include <launchdarkly/config/detail/built/http_properties.hpp>
#include <launchdarkly/config/detail/built/service_endpoints.hpp>
#include <launchdarkly/logger.hpp>

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
           built::ServiceEndpoints endpoints,
           built::Events events,
           std::optional<std::string> application_tag,
           built::DataSourceConfig<SDK> data_source_config,
           built::HttpProperties http_properties);

    [[nodiscard]] std::string const& SdkKey() const;

    [[nodiscard]] built::ServiceEndpoints const& ServiceEndpoints() const;

    [[nodiscard]] built::Events const& Events() const;

    [[nodiscard]] std::optional<std::string> const& ApplicationTag() const;

    built::DataSourceConfig<SDK> const& DataSourceConfig() const;

    [[nodiscard]] built::HttpProperties const& HttpProperties() const;

    [[nodiscard]] bool Offline() const;

    launchdarkly::Logger Logger() const;

   private:
    std::string sdk_key_;
    bool offline_;
    launchdarkly::Logger logger_;
    built::ServiceEndpoints service_endpoints_;
    std::optional<std::string> application_tag_;
    built::Events events_;
    built::DataSourceConfig<SDK> data_source_config_;
    built::HttpProperties http_properties_;
};

}  // namespace launchdarkly::config::detail
