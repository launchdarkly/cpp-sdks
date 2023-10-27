#pragma once

#include <launchdarkly/config/shared/builders/data_system/background_sync_builder.hpp>
#include <launchdarkly/config/shared/built/data_system/data_system_config.hpp>

#include <launchdarkly/config/shared/builders/data_system/bootstrap_builder.hpp>

#include <launchdarkly/config/shared/defaults.hpp>
#include <launchdarkly/config/shared/sdks.hpp>

#include <chrono>
#include <optional>
#include <type_traits>
#include <vector>

namespace launchdarkly::config::shared::builders {

/**
 * Used to construct a DataSourcesConfiguration for the specified SDK type.
 * @tparam SDK ClientSDK or ServerSDK.
 */
template <typename SDK>
class DataSystemBuilder;

/** Not used in client-side SDK yet. */
template <>
class DataSystemBuilder<ClientSDK> {
   public:
    DataSystemBuilder() {}
};

template <>
class DataSystemBuilder<ServerSDK> {
   public:
    using BackgroundSyncBuilder = BackgroundSyncBuilder<ServerSDK>;
    //  using LazyLoadBuilder = LazyLoadBuilder<ServerSDK>;

    DataSystemBuilder();

    // DataSystemBuilder& LazyLoad(LazyLoadBuilder lazy_load);

    DataSystemBuilder& BackgroundSync(BackgroundSyncBuilder background_sync);

    [[nodiscard]] built::DataSystemConfig<ServerSDK> Build() const;

   private:
    built::DataSystemConfig<ServerSDK> config_;
};

}  // namespace launchdarkly::config::shared::builders
