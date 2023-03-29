#include "config/detail/config_builder.hpp"

namespace launchdarkly::config::detail {
template <typename SDK>
ConfigBuilder<SDK>::ConfigBuilder(std::string sdk_key)
    : sdk_key_(std::move(sdk_key)),
      offline_(false),
      service_endpoints_builder_(std::nullopt) {}

template <typename SDK>
ConfigBuilder<SDK>& ConfigBuilder<SDK>::service_endpoints(
    detail::EndpointsBuilder<SDK> builder) {
    service_endpoints_builder_ = std::move(builder);
    return *this;
}

template <typename SDK>
typename ConfigBuilder<SDK>::ConfigType ConfigBuilder<SDK>::build() const {
    return {
        sdk_key_, offline_, service_endpoints_builder_.value_or()
    }
}

}  // namespace launchdarkly::config::detail
