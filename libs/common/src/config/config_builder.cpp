#include "config/detail/config_builder.hpp"
#include "config/detail/defaults.hpp"

namespace launchdarkly::config::detail {
template <typename SDK>
ConfigBuilder<SDK>::ConfigBuilder(std::string sdk_key)
    : sdk_key_(std::move(sdk_key)),
      offline_(std::nullopt),
      service_endpoints_builder_(std::nullopt) {}

template <typename SDK>
ConfigBuilder<SDK>& ConfigBuilder<SDK>::service_endpoints(
    detail::EndpointsBuilder<SDK> builder) {
    service_endpoints_builder_ = std::move(builder);
    return *this;
}

template <typename SDK>
ConfigBuilder<SDK>& ConfigBuilder<SDK>::offline(bool offline) {
    offline_ = offline;
    return *this;
}

template <typename SDK>
typename ConfigBuilder<SDK>::ConfigType ConfigBuilder<SDK>::build() const {
    return {sdk_key_, offline_.value_or(Defaults<detail::AnySDK>::offline()),
            service_endpoints_builder_.value_or(
                ConfigBuilder<SDK>::EndpointsBuilder())};
}

template class ConfigBuilder<detail::ClientSDK>;
template class ConfigBuilder<detail::ServerSDK>;

}  // namespace launchdarkly::config::detail
