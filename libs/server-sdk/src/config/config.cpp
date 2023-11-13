#include <launchdarkly/server_side/config/config.hpp>

namespace launchdarkly::server_side {

Config::Config(std::string sdk_key,
               bool offline,
               config::shared::built::Logging logging,
               config::shared::built::ServiceEndpoints service_endpoints,
               config::shared::built::Events events,
               std::optional<std::string> application_tag,
               config::shared::built::DataSystemConfig<SDK> data_system_config,
               config::shared::built::HttpProperties http_properties)
    : sdk_key_(std::move(sdk_key)),
      logging_(std::move(logging)),
      offline_(offline),
      service_endpoints_(std::move(service_endpoints)),
      events_(std::move(events)),
      application_tag_(std::move(application_tag)),
      data_system_config_(std::move(data_system_config)),
      http_properties_(std::move(http_properties)) {}

std::string const& Config::SdkKey() const {
    return sdk_key_;
}

config::shared::built::ServiceEndpoints const& Config::ServiceEndpoints()
    const {
    return service_endpoints_;
}

config::shared::built::Events const& Config::Events() const {
    return events_;
}

std::optional<std::string> const& Config::ApplicationTag() const {
    return application_tag_;
}

config::shared::built::DataSystemConfig<config::shared::ServerSDK> const&
Config::DataSystemConfig() const {
    return data_system_config_;
}

config::shared::built::HttpProperties const& Config::HttpProperties() const {
    return http_properties_;
}

bool Config::Offline() const {
    return offline_;
}

config::shared::built::Logging const& Config::Logging() const {
    return logging_;
}

}  // namespace launchdarkly::server_side
