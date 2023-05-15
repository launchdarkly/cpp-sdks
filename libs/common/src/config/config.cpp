#include <utility>

#include "launchdarkly/config/shared/config.hpp"

namespace launchdarkly::config {
template <typename SDK>
Config<SDK>::Config(std::string sdk_key,
                    bool offline,
                    shared::built::Logging logging,
                    shared::built::ServiceEndpoints service_endpoints,
                    shared::built::Events events,
                    std::optional<std::string> application_tag,
                    shared::built::DataSourceConfig<SDK> data_source_config,
                    shared::built::HttpProperties http_properties)
    : sdk_key_(std::move(sdk_key)),
      logging_(std::move(logging)),
      offline_(offline),
      service_endpoints_(std::move(service_endpoints)),
      events_(std::move(events)),
      application_tag_(std::move(application_tag)),
      data_source_config_(std::move(data_source_config)),
      http_properties_(std::move(http_properties)) {}

template <typename SDK>
std::string const& Config<SDK>::SdkKey() const {
    return sdk_key_;
}

template <typename SDK>
shared::built::ServiceEndpoints const& Config<SDK>::ServiceEndpoints() const {
    return service_endpoints_;
}

template <typename SDK>
shared::built::Events const& Config<SDK>::Events() const {
    return events_;
}

template <typename SDK>
std::optional<std::string> const& Config<SDK>::ApplicationTag() const {
    return application_tag_;
}

template <typename SDK>
shared::built::DataSourceConfig<SDK> const& Config<SDK>::DataSourceConfig()
    const {
    return data_source_config_;
}

template <typename SDK>
shared::built::HttpProperties const& Config<SDK>::HttpProperties() const {
    return http_properties_;
}

template <typename SDK>
bool Config<SDK>::Offline() const {
    return offline_;
}

template <typename SDK>
shared::built::Logging const& Config<SDK>::Logging() const {
    return logging_;
}

template class Config<config::ClientSDK>;
template class Config<config::ServerSDK>;

}  // namespace launchdarkly::config
