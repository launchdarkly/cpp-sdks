#pragma once

#include <launchdarkly/server_side/config/built/all_built.hpp>
#include <launchdarkly/server_side/config/built/data_system/data_system_config.hpp>

namespace launchdarkly::server_side {

struct Config {
   public:
    Config(std::string sdk_key,
           config::built::Logging logging,
           config::built::ServiceEndpoints endpoints,
           config::built::Events events,
           std::optional<std::string> application_tag,
           config::built::DataSystemConfig data_system_config,
           config::built::HttpProperties http_properties);

    [[nodiscard]] std::string const& SdkKey() const;

    [[nodiscard]] config::built::ServiceEndpoints const& ServiceEndpoints()
        const;

    [[nodiscard]] config::built::Events const& Events() const;

    [[nodiscard]] std::optional<std::string> const& ApplicationTag() const;

    config::built::DataSystemConfig const& DataSystemConfig() const;

    [[nodiscard]] config::built::HttpProperties const& HttpProperties() const;

    [[nodiscard]] config::built::Logging const& Logging() const;

   private:
    std::string sdk_key_;
    bool offline_;
    config::built::Logging logging_;
    config::built::ServiceEndpoints service_endpoints_;
    std::optional<std::string> application_tag_;
    config::built::Events events_;
    config::built::DataSystemConfig data_system_config_;
    config::built::HttpProperties http_properties_;
};
}  // namespace launchdarkly::server_side
