#pragma once

#include <launchdarkly/config/shared/built/events.hpp>
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <launchdarkly/config/shared/built/logging.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>

#include <launchdarkly/server_side/config/built/data_system/data_system_config.hpp>

namespace launchdarkly::server_side {

struct Config {
   public:
    Config(std::string sdk_key,
           launchdarkly::config::shared::built::Logging logging,
           launchdarkly::config::shared::built::ServiceEndpoints endpoints,
           launchdarkly::config::shared::built::Events events,
           std::optional<std::string> application_tag,
           config::built::DataSystemConfig data_system_config,
           launchdarkly::config::shared::built::HttpProperties http_properties);

    [[nodiscard]] std::string const& SdkKey() const;

    [[nodiscard]] launchdarkly::config::shared::built::ServiceEndpoints const&
    ServiceEndpoints() const;

    [[nodiscard]] launchdarkly::config::shared::built::Events const& Events()
        const;

    [[nodiscard]] std::optional<std::string> const& ApplicationTag() const;

    config::built::DataSystemConfig const& DataSystemConfig() const;

    [[nodiscard]] launchdarkly::config::shared::built::HttpProperties const&
    HttpProperties() const;

    [[nodiscard]] launchdarkly::config::shared::built::Logging const& Logging()
        const;

   private:
    std::string sdk_key_;
    bool offline_;
    launchdarkly::config::shared::built::Logging logging_;
    launchdarkly::config::shared::built::ServiceEndpoints service_endpoints_;
    std::optional<std::string> application_tag_;
    launchdarkly::config::shared::built::Events events_;
    config::built::DataSystemConfig data_system_config_;
    launchdarkly::config::shared::built::HttpProperties http_properties_;
};
}  // namespace launchdarkly::server_side
