#pragma once

#include <launchdarkly/server_side/config/builders/data_system/background_sync_builder.hpp>
#include <launchdarkly/server_side/config/builders/data_system/lazy_load_builder.hpp>
#include <launchdarkly/server_side/config/built/data_system/data_system_config.hpp>

#include <launchdarkly/error.hpp>

namespace launchdarkly::server_side::config::builders {

class DataSystemBuilder {
   public:
    DataSystemBuilder();
    using BackgroundSync = BackgroundSyncBuilder;
    using LazyLoad = LazyLoadBuilder;

    /**
     * @brief Alias for Enabled(false).
     * @return Reference to this.
     */
    DataSystemBuilder& Disable();

    /**
     * @brief Specifies if the data system is enabled or disabled.
     * If disabled, the configured method won't be used. Defaults to true.
     * @param enabled If the data system is enabled.
     * @return Reference to this.
     */
    DataSystemBuilder& Enabled(bool enabled);

    /**
     * @brief Configures the Background Sync data system. In this system,
     * the SDK periodically receives updates from LaunchDarkly servers and
     * stores them in an in-memory cache. This is the default data system.
     * @param bg_sync Background Sync configuration.
     * @return Reference to this.
     */
    DataSystemBuilder& Method(BackgroundSync bg_sync);

    /**
     * @brief Configures the Lazy Load data system. In this system, the SDK
     * pulls data on demand from a configured source, caching responses in
     * memory for a configurable duration.
     * @param lazy_load Lazy Load configuration.
     * @return Reference to this.
     */
    DataSystemBuilder& Method(LazyLoad lazy_load);

    [[nodiscard]] tl::expected<built::DataSystemConfig, Error> Build() const;

   private:
    std::optional<std::variant<BackgroundSync, LazyLoad>> method_builder_;
    built::DataSystemConfig config_;
};

}  // namespace launchdarkly::server_side::config::builders
