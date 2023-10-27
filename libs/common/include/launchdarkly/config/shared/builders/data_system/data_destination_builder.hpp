#pragma once

#include <launchdarkly/config/shared/built/data_system/data_destination_config.hpp>
#include <launchdarkly/config/shared/sdks.hpp>

#include <chrono>
#include <optional>
#include <type_traits>
#include <variant>

namespace launchdarkly::config::shared::builders {

template <typename SDK>
struct DataDestinationBuilder {};

template <>
struct DataDestinationBuilder<ClientSDK> {};

template <>
struct DataDestinationBuilder<ServerSDK> {
    DataDestinationBuilder();

    [[nodiscard]] config::shared::built::DataDestinationConfig<ServerSDK>
    Build() const;

   private:
    built::DataDestinationConfig<ServerSDK> config_;
};

}  // namespace launchdarkly::config::shared::builders
