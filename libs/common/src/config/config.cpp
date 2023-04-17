#include <utility>

#include "config/detail/config.hpp"

#include "config/detail/sdks.hpp"
namespace launchdarkly::config::detail {
template <typename SDK>
Config<SDK>::Config(std::string sdk_key,
                    bool offline,
                    Logger logger,
                    ServiceEndpoints service_endpoints,
                    Events events,
                    std::optional<std::string> application_tag,
                    DataSourceConfig<SDK> data_source_config,
                    detail::HttpProperties http_properties)
    : sdk_key_(std::move(sdk_key)),
      logger_(std::move(logger)),
      offline_(offline),
      service_endpoints_(std::move(service_endpoints)),
      events_(std::move(events)),
      application_tag_(std::move(application_tag)),
      data_source_config_(std::move(data_source_config)),
      http_properties_(std::move(http_properties)) {}

template <typename SDK>
std::string const& Config<SDK>::sdk_key() const {
    return sdk_key_;
}

template <typename SDK>
ServiceEndpoints const& Config<SDK>::service_endpoints() const {
    return service_endpoints_;
}

template <typename SDK>
Events const& Config<SDK>::events_config() const {
    return events_;
}

template <typename SDK>
std::optional<std::string> const& Config<SDK>::application_tag() const {
    return application_tag_;
}

template <typename SDK>
DataSourceConfig<SDK> const& Config<SDK>::data_source_config() const {
    return data_source_config_;
}

template <typename SDK>
HttpProperties const& Config<SDK>::http_properties() const {
    return http_properties_;
}

template <typename SDK>
bool Config<SDK>::offline() const {
    return false;
}

template <typename SDK>
Logger Config<SDK>::take_logger() {
    return std::move(logger_);
}

template class Config<detail::ClientSDK>;
template class Config<detail::ServerSDK>;

}  // namespace launchdarkly::config::detail
