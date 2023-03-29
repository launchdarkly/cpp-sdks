#pragma once

#include <optional>
#include <string>
#include "config/detail/config.hpp"
#include "config/detail/endpoints_builder.hpp"

namespace launchdarkly::config::detail {

template <typename SDK>
class ConfigBuilder {
   public:
    using EndpointsBuilder = detail::EndpointsBuilder<SDK>;
    using ConfigType = detail::Config<SDK>;
    ConfigBuilder(std::string sdk_key);
    ConfigBuilder& service_endpoints(detail::EndpointsBuilder<SDK> builder);
    ConfigType build() const;

   private:
    std::string sdk_key_;
    bool offline_;
    std::optional<EndpointsBuilder> service_endpoints_builder_;
};

}

}  // namespace launchdarkly::config::detail
