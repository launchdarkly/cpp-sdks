#include <gtest/gtest.h>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/logging/null_logger.hpp>
#include <launchdarkly/server_side/client.hpp>
#include <launchdarkly/server_side/config/config_builder.hpp>

#include <data_systems/background_sync/sources/streaming/streaming_data_source.hpp>
#include <map>

using namespace launchdarkly;
using namespace launchdarkly::server_side;

class ConfigBuilderTest
    : public ::testing::
          Test {  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
   protected:
    launchdarkly::Logger logger;
    ConfigBuilderTest() : logger(launchdarkly::logging::NullLogger()) {}
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

    auto const streaming =
        DataSystemBuilder::BackgroundSyncBuilder::Streaming()
            .InitialReconnectDelay(delay);

    builder.DataSystem().BackgroundSync(
        DataSystemBuilder::BackgroundSyncBuilder()
            .Source(streaming));

    auto cfg = builder.Build();

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

TEST_F(ConfigBuilderTest,
       DefaultConstruction_ServerConfig_UsesDefaultHttpProperties) {
    using namespace launchdarkly::server_side;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_EQ(cfg->HttpProperties(), Defaults::HttpProperties());
}
