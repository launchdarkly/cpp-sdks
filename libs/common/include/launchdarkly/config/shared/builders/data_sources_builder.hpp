#pragma once

#include <chrono>
#include <optional>
#include <type_traits>
#include <vector>

#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/defaults.hpp>
#include <launchdarkly/config/shared/sdks.hpp>

namespace launchdarkly::config::shared::builders {

/**
 * Used to construct a DataSourcesConfiguration for the specified SDK type.
 * @tparam SDK ClientSDK or ServerSDK.
 */
template <typename SDK>
class DataSourcesBuilder;

template <>
class DataSourcesBuilder<ClientSDK> {
   public:
    DataSourcesBuilder() {}
};

template <>
class DataSourcesBuilder<ServerSDK> {
   public:
    DataSourcesBuilder();

    DataSourcesBuilder& Source();

    DataSourcesBuilder& Destination();
    [[nodiscard]] built::DataSourceConfig<ClientSDK> Build() const;

   private:
    std::vector<DataSourceBuilder<ServerSDK>> builders_;
};

}  // namespace launchdarkly::config::shared::builders
