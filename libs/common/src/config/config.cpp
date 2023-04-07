#include "config/detail/config.hpp"

#include "config/detail/sdks.hpp"
namespace launchdarkly::config::detail {
template <typename SDK>
Config<SDK>::Config(std::string sdk_key,
                    bool offline,
                    detail::HostsBuilder<SDK> service_endpoints_builder,
                    detail::EventsBuilder<SDK> events_builder,
                    std::optional<std::string> application_tag)
    : sdk_key(std::move(sdk_key)),
      offline(offline),
      hosts_builder(std::move(service_endpoints_builder)),
      events_builder(std::move(events_builder)),
      application_tag(std::move(application_tag)) {}

template class Config<detail::ClientSDK>;
template class Config<detail::ServerSDK>;

}  // namespace launchdarkly::config::detail
