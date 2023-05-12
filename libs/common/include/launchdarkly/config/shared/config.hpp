#pragma once

#include <launchdarkly/config/shared/builders/endpoints_builder.hpp>
#include <launchdarkly/config/shared/builders/events_builder.hpp>
#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/built/events.hpp>
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <launchdarkly/config/shared/built/logging_config.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>

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
           config::shared::built::LoggingConfig logging_config,
           config::shared::built::ServiceEndpoints endpoints,
           config::shared::built::Events events,
           std::optional<std::string> application_tag,
           config::shared::built::DataSourceConfig<SDK> data_source_config,
           config::shared::built::HttpProperties http_properties);

    [[nodiscard]] std::string const& SdkKey() const;

    [[nodiscard]] config::shared::built::ServiceEndpoints const&
    ServiceEndpoints() const;

    [[nodiscard]] config::shared::built::Events const& Events() const;

    [[nodiscard]] std::optional<std::string> const& ApplicationTag() const;

    config::shared::built::DataSourceConfig<SDK> const& DataSourceConfig()
        const;

    [[nodiscard]] config::shared::built::HttpProperties const& HttpProperties()
        const;

    [[nodiscard]] bool Offline() const;

    launchdarkly::Logger Logger();

   private:
    std::string sdk_key_;
    bool offline_;
    config::shared::built::LoggingConfig logging_config_;
    config::shared::built::ServiceEndpoints service_endpoints_;
    std::optional<std::string> application_tag_;
    config::shared::built::Events events_;
    config::shared::built::DataSourceConfig<SDK> data_source_config_;
    config::shared::built::HttpProperties http_properties_;
};

}  // namespace launchdarkly::config::detail
