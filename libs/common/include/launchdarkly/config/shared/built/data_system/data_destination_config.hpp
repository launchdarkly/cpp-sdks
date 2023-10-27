#pragma once

#include <launchdarkly/config/shared/sdks.hpp>

namespace launchdarkly::config::shared::built {

template <typename SDK>
struct DataDestinationConfig {};

template <>
struct DataDestinationConfig<ClientSDK> {};

template <>
struct DataDestinationConfig<ServerSDK> {};

}  // namespace launchdarkly::config::shared::built
