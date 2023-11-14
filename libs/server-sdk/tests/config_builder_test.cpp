#include <gtest/gtest.h>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/logging/null_logger.hpp>
#include <launchdarkly/server_side/client.hpp>
#include <launchdarkly/server_side/config/config_builder.hpp>

#include "data_systems/background_sync/sources/streaming/streaming_data_source.hpp"

using namespace launchdarkly;
using namespace launchdarkly::server_side;

class ConfigBuilderTest
    : public ::testing::
          Test {  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
   protected:
    Logger logger;
    ConfigBuilderTest() : logger(logging::NullLogger()) {}
};

TEST_F(ConfigBuilderTest, DefaultConstruction_ServerConfig) {
    using namespace launchdarkly::server_side;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_TRUE(cfg);
    ASSERT_EQ(cfg->SdkKey(), "sdk-123");
}

TEST_F(ConfigBuilderTest,
       DefaultConstruction_ServerConfig_UsesDefaulDataSystemConfig) {
    using namespace launchdarkly::server_side;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();

    auto background_sync_config =
        std::get<launchdarkly::config::shared::built::BackgroundSyncConfig<
            launchdarkly::config::shared::ServerSDK>>(
            cfg->DataSystemConfig().system_);

    auto streaming_config =
        std::get<launchdarkly::config::shared::built::StreamingConfig<
            launchdarkly::config::shared::ServerSDK>>(
            background_sync_config.source_.method);

    // Should be streaming with a 1 second initial delay.
    EXPECT_EQ(std::chrono::milliseconds{1000},
              streaming_config.initial_reconnect_delay);
}

TEST_F(ConfigBuilderTest, ServerConfig_CanModifyStreamReconnectDelay) {
    using namespace launchdarkly::server_side;
    ConfigBuilder builder("sdk-123");

    auto const delay = std::chrono::seconds{5};

    auto const streaming_connection =
        DataSystemBuilder::BackgroundSync::Streaming().InitialReconnectDelay(
            delay);

    auto const background_sync =
        DataSystemBuilder::BackgroundSync().Synchronizer(streaming_connection);

    builder.DataSystem().Method(background_sync);

    auto const cfg = builder.Build();

    EXPECT_EQ(
        delay,
        std::get<launchdarkly::config::shared::built::StreamingConfig<
            launchdarkly::config::shared::ServerSDK>>(
            std::get<launchdarkly::config::shared::built::BackgroundSyncConfig<
                launchdarkly::config::shared::ServerSDK>>(
                cfg->DataSystemConfig().system_)
                .source_.method)
            .initial_reconnect_delay);
}

TEST_F(ConfigBuilderTest, CanDisableDataSystem) {
    using namespace launchdarkly::server_side;
    ConfigBuilder builder("sdk-123");

    auto const cfg1 = builder.Build();
    EXPECT_FALSE(cfg1->DataSystemConfig().disabled);

    builder.DataSystem().Disabled(true);
    auto const cfg2 = builder.Build();
    EXPECT_TRUE(cfg2->DataSystemConfig().disabled);
}

TEST_F(ConfigBuilderTest,
       DefaultConstruction_ServerConfig_UsesDefaultHttpProperties) {
    using namespace launchdarkly::server_side;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_EQ(cfg->HttpProperties(), Defaults::HttpProperties());
}
