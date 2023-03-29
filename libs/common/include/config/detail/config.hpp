#pragma once

#include "config/detail/endpoints_builder.hpp"

namespace launchdarkly::config::detail {

template <typename SDK>
struct Config {
    std::string sdk_key;
    detail::EndpointsBuilder<SDK> service_endpoints_builder;
    bool offline;
};

}  // namespace launchdarkly::config::detail
