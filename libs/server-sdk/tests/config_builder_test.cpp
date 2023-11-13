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
       DefaultConstruction_ServerConfig_UsesDefaulDataSourceConfig) {
    using namespace launchdarkly::server_side;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();

    // Should be streaming with a 1 second initial retry;
    EXPECT_EQ(std::chrono::milliseconds{1000},
              std::get<launchdarkly::config::shared::built::StreamingConfig<
                  launchdarkly::config::shared::ServerSDK>>(
                  cfg->DataSourceConfig().method)
                  .initial_reconnect_delay);
}

TEST_F(ConfigBuilderTest, ServerConfig_CanSetDataSystem) {
    using namespace launchdarkly::server_side;
    ConfigBuilder builder("sdk-123");

    auto const streaming =
        DataSystemBuilder::BackgroundSyncBuilder::Streaming()
            .InitialReconnectDelay(std::chrono::milliseconds{5000});

    auto const mirror =
        DataSystemBuilder::BackgroundSyncBuilder::DataDestinationBuilder();

    builder.DataSystem().BackgroundSync(
        DataSystemBuilder::BackgroundSyncBuilder()
            .Source(streaming)
            .Destination(mirror));

    auto cfg = builder.Build();

    EXPECT_EQ(std::chrono::milliseconds{5000},
              std::get<launchdarkly::config::shared::built::StreamingConfig<
                  launchdarkly::config::shared::ServerSDK>>(
                  cfg->DataSourceConfig().method)
                  .initial_reconnect_delay);
}

TEST_F(ConfigBuilderTest,
       DefaultConstruction_ServerConfig_UsesDefaultHttpProperties) {
    using namespace launchdarkly::server_side;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_EQ(cfg->HttpProperties(), Defaults::HttpProperties());
}
