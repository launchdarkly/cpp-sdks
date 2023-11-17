#include <launchdarkly/server_side/config/config.hpp>

namespace launchdarkly::server_side {

using namespace launchdarkly::config::shared;

Config::Config(std::string sdk_key,
               built::Logging logging,
               built::ServiceEndpoints service_endpoints,
               built::Events events,
               std::optional<std::string> application_tag,
               launchdarkly::server_side::config::built::DataSystemConfig
                   data_system_config,
               built::HttpProperties http_properties)
    : sdk_key_(std::move(sdk_key)),
      logging_(std::move(logging)),
      service_endpoints_(std::move(service_endpoints)),
      events_(std::move(events)),
      application_tag_(std::move(application_tag)),
      data_system_config_(std::move(data_system_config)),
      http_properties_(std::move(http_properties)) {}

std::string const& Config::SdkKey() const {
    return sdk_key_;
}

built::ServiceEndpoints const& Config::ServiceEndpoints() const {
    return service_endpoints_;
}

built::Events const& Config::Events() const {
    return events_;
}

std::optional<std::string> const& Config::ApplicationTag() const {
    return application_tag_;
}

launchdarkly::server_side::config::built::DataSystemConfig const&
Config::DataSystemConfig() const {
    return data_system_config_;
}

built::HttpProperties const& Config::HttpProperties() const {
    return http_properties_;
}

built::Logging const& Config::Logging() const {
    return logging_;
}

}  // namespace launchdarkly::server_side
