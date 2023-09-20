#pragma once

#include <chrono>
#include <optional>
#include <type_traits>
#include <vector>

#include <launchdarkly/config/shared/builders/data_source_builder.hpp>

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

class BootstrapBuilder {
   public:
    BootstrapBuilder();
    enum class Order { ConsistentFirst = 0, Assigned = 1, Random = 2 };

    using SeedType = std::int64_t;

    BootstrapBuilder& Order(Order order);

    BootstrapBuilder& RandomSeed(SeedType seed);

   private:
    enum Order order_;
    std::optional<SeedType> seed_;
};

template <>
class DataSourcesBuilder<ServerSDK> {
   public:
    DataSourcesBuilder();

    DataSourceBuilder<ServerSDK>& Source();

    DataSourcesBuilder& Destination();

    BootstrapBuilder& Bootstrap();

    [[nodiscard]] built::DataSourceConfig<ClientSDK> Build() const;

   private:
    BootstrapBuilder bootstrap_;
    std::vector<DataSourceBuilder<ServerSDK>> sources_;
};

}  // namespace launchdarkly::config::shared::builders
