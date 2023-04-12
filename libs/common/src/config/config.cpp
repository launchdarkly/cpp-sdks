#include "config/detail/config.hpp"

#include "config/detail/sdks.hpp"
namespace launchdarkly::config::detail {
template <typename SDK>
Config<SDK>::Config(std::string sdk_key,
                    bool offline,
                    detail::EndpointsBuilder<SDK> service_endpoints_builder,
                    std::optional<std::string> application_tag,
                    DataSourceConfig<SDK> data_source_config)
    : sdk_key(std::move(sdk_key)),
      offline(offline),
      service_endpoints_builder(std::move(service_endpoints_builder)),
      application_tag(std::move(application_tag)),
      data_source_config(std::move(data_source_config)) {}

template class Config<detail::ClientSDK>;
template class Config<detail::ServerSDK>;

}  // namespace launchdarkly::config::detail
