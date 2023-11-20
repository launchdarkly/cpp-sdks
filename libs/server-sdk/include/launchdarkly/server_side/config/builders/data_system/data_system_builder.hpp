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

    DataSystemBuilder& Disabled(bool disabled);

    DataSystemBuilder& Method(BackgroundSync bg_sync);
    DataSystemBuilder& Method(LazyLoad lazy_load);

    [[nodiscard]] tl::expected<built::DataSystemConfig, Error> Build() const;

   private:
    std::optional<std::variant<BackgroundSync, LazyLoad>> method_builder_;
    built::DataSystemConfig config_;
};

}  // namespace launchdarkly::server_side::config::builders
