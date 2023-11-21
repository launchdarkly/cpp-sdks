#include <gtest/gtest.h>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/logging/null_logger.hpp>
#include <launchdarkly/server_side/client.hpp>
#include <launchdarkly/server_side/config/config_builder.hpp>

#include "config/builders/data_system/defaults.hpp"
#include "data_systems/background_sync/sources/streaming/streaming_data_source.hpp"

using namespace launchdarkly;
using namespace launchdarkly::server_side;
using namespace launchdarkly::server_side::config;

class ConfigBuilderTest
    : public ::testing::
          Test {  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
   protected:
    Logger logger;
    ConfigBuilderTest() : logger(logging::NullLogger()) {}
};

TEST_F(ConfigBuilderTest, DefaultConstruction_Succeeds) {
    // It's important that the SDK's configuration can be constructed with
    // nothing more than an SDK key.
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_TRUE(cfg);
    ASSERT_EQ(cfg->SdkKey(), "sdk-123");
}

TEST_F(ConfigBuilderTest, DefaultConstruction_StreamingDefaultsAreUsed) {
    // Sanity check that the default server-side config uses
    // the streaming data source.

    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();

    ASSERT_TRUE(std::holds_alternative<built::BackgroundSyncConfig>(
        cfg->DataSystemConfig().system_));

    auto const bg_sync_config =
        std::get<built::BackgroundSyncConfig>(cfg->DataSystemConfig().system_);

    ASSERT_TRUE(
        std::holds_alternative<built::BackgroundSyncConfig::StreamingConfig>(
            bg_sync_config.synchronizer_));

    auto const streaming_config =
        std::get<built::BackgroundSyncConfig::StreamingConfig>(
            bg_sync_config.synchronizer_);

    EXPECT_EQ(streaming_config, Defaults::SynchronizerConfig());
}

TEST_F(ConfigBuilderTest, DefaultConstruction_HttpPropertyDefaultsAreUsed) {
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_EQ(cfg->HttpProperties(),
              ::launchdarkly::config::shared::Defaults<
                  launchdarkly::config::shared::ServerSDK>::Defaults::
                  HttpProperties());
}

TEST_F(ConfigBuilderTest, DefaultConstruction_ServiceEndpointDefaultsAreUsed) {
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_EQ(cfg->ServiceEndpoints(),
              ::launchdarkly::config::shared::Defaults<
                  launchdarkly::config::shared::ServerSDK>::Defaults::
                  ServiceEndpoints());
}

TEST_F(ConfigBuilderTest, DefaultConstruction_EventDefaultsAreUsed) {
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_EQ(cfg->Events(),
              ::launchdarkly::config::shared::Defaults<
                  launchdarkly::config::shared::ServerSDK>::Defaults::Events());
}

TEST_F(ConfigBuilderTest, CanDisableDataSystem) {
    ConfigBuilder builder("sdk-123");

    // First establish that the data system is enabled.
    auto const cfg1 = builder.Build();
    EXPECT_FALSE(cfg1->DataSystemConfig().disabled);

    builder.DataSystem().Disabled(true);
    auto const cfg2 = builder.Build();
    EXPECT_TRUE(cfg2->DataSystemConfig().disabled);
}
