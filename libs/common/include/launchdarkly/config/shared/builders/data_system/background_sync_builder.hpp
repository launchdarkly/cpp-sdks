#pragma once

#include <launchdarkly/config/shared/builders/data_source_builder.hpp>
#include <launchdarkly/config/shared/builders/data_system/bootstrap_builder.hpp>
#include <launchdarkly/config/shared/builders/data_system/data_destination_builder.hpp>

#include <launchdarkly/config/shared/defaults.hpp>
#include <launchdarkly/config/shared/sdks.hpp>

#include <chrono>
#include <optional>
#include <type_traits>
#include <variant>

namespace launchdarkly::config::shared::builders {

template <typename SDK>
struct BackgroundSyncBuilder {};

template <>
struct BackgroundSyncBuilder<ClientSDK> {};

template <>
struct BackgroundSyncBuilder<ServerSDK> {
    using Streaming = DataSourceBuilder<ServerSDK>::Streaming;
    using Polling = DataSourceBuilder<ServerSDK>::Polling;

    using DataDestinationBuilder = DataDestinationBuilder<ServerSDK>;

    BackgroundSyncBuilder();

    BootstrapBuilder& Bootstrapper();

    BackgroundSyncBuilder& Synchronizer(Streaming source);
    BackgroundSyncBuilder& Synchronizer(Polling source);

    BackgroundSyncBuilder& Destination(DataDestinationBuilder destination);

    [[nodiscard]] built::BackgroundSyncConfig<ServerSDK> Build() const;

   private:
    BootstrapBuilder bootstrap_builder_;
    built::BackgroundSyncConfig<ServerSDK> config_;

    //    /* Will be the default LaunchDarkly bootstrapper or a custom one
    //    provided
    //     * by the user. */
    //    BootstrapBuilder primary_bootstrapper_;
    //
    //    /* Optional, as appropriate fallbacks might not be available or
    //    wanted. */ std::optional<BootstrapBuilder> fallback_bootstrapper_;
    //
    //    DataSourceBuilder source_;
    //
    //    /* Optional, as there may be no need to mirror data anywhere. */
    //    std::optional<DataDestinationBuilder> destination_;
};

}  // namespace launchdarkly::config::shared::builders
