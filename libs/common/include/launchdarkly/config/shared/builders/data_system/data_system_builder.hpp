#pragma once

#include <launchdarkly/config/shared/builders/data_system/background_sync_builder.hpp>
#include <launchdarkly/config/shared/builders/data_system/lazy_load_builder.hpp>

#include <launchdarkly/config/shared/built/data_system/data_system_config.hpp>
#include <launchdarkly/config/shared/defaults.hpp>
#include <launchdarkly/config/shared/sdks.hpp>

#include <launchdarkly/error.hpp>

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
    DataSystemBuilder();
    using BackgroundSync = BackgroundSyncBuilder<ServerSDK>;
    using LazyLoad = LazyLoadBuilder;

    DataSystemBuilder& Disabled(bool disabled);

    DataSystemBuilder& Method(BackgroundSync bg_sync);
    DataSystemBuilder& Method(LazyLoad lazy_load);

    [[nodiscard]] tl::expected<built::DataSystemConfig<ServerSDK>, Error>
    Build() const;

   private:
    std::optional<
        tl::expected<std::variant<built::LazyLoadConfig,
                                  built::BackgroundSyncConfig<ServerSDK>>,
                     Error>>
        method_config_;
    std::optional<std::variant<BackgroundSync, LazyLoad>> method_builder_;
    built::DataSystemConfig<ServerSDK> config_;
};

}  // namespace launchdarkly::config::shared::builders
