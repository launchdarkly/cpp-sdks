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

class ConfigBuilderTest : public ::testing::Test {
    // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
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

TEST_F(ConfigBuilderTest, CanSetStreamingPayloadFilterKey) {
    ConfigBuilder builder("sdk-123");
    builder.DataSystem().Method(
        builders::DataSystemBuilder::BackgroundSync().Synchronizer(
            builders::BackgroundSyncBuilder::Streaming().Filter("foo")));

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

    EXPECT_EQ(streaming_config.filter_key, "foo");
}

TEST_F(ConfigBuilderTest, CanSetPollingPayloadFilterKey) {
    ConfigBuilder builder("sdk-123");
    builder.DataSystem().Method(
        builders::DataSystemBuilder::BackgroundSync().Synchronizer(
            builders::BackgroundSyncBuilder::Polling().Filter("foo")));

    auto cfg = builder.Build();

    ASSERT_TRUE(std::holds_alternative<built::BackgroundSyncConfig>(
        cfg->DataSystemConfig().system_));

    auto const bg_sync_config =
        std::get<built::BackgroundSyncConfig>(cfg->DataSystemConfig().system_);

    ASSERT_TRUE(
        std::holds_alternative<built::BackgroundSyncConfig::PollingConfig>(
            bg_sync_config.synchronizer_));

    auto const polling_config =
        std::get<built::BackgroundSyncConfig::PollingConfig>(
            bg_sync_config.synchronizer_);

    EXPECT_EQ(polling_config.filter_key, "foo");
}

TEST_F(ConfigBuilderTest, FDv2_DefaultsAreUsed) {
    ConfigBuilder builder("sdk-123");
    builder.DataSystem().Method(builders::DataSystemBuilder::FDv2());

    auto cfg = builder.Build();

    ASSERT_TRUE(std::holds_alternative<built::FDv2Config>(
        cfg->DataSystemConfig().system_));
    auto const fdv2_config =
        std::get<built::FDv2Config>(cfg->DataSystemConfig().system_);

    EXPECT_EQ(fdv2_config.streaming, Defaults::FDv2StreamingConfig());
    EXPECT_EQ(fdv2_config.polling.poll_interval,
              Defaults::FDv2PollingConfig().poll_interval);
    ASSERT_TRUE(fdv2_config.fdv1_fallback.has_value());
    EXPECT_EQ(*fdv2_config.fdv1_fallback, Defaults::FDv2StreamingConfig());
    EXPECT_EQ(fdv2_config.fallback_timeout, std::chrono::minutes{2});
    EXPECT_EQ(fdv2_config.recovery_timeout, std::chrono::minutes{5});
}

TEST_F(ConfigBuilderTest, FDv2_StreamingFilterFlowsThrough) {
    ConfigBuilder builder("sdk-123");
    builder.DataSystem().Method(builders::DataSystemBuilder::FDv2().Streaming(
        builders::FDv2Builder::StreamingSource().Filter("flag-subset")));

    auto cfg = builder.Build();
    auto const fdv2_config =
        std::get<built::FDv2Config>(cfg->DataSystemConfig().system_);

    EXPECT_EQ(fdv2_config.streaming.filter_key, "flag-subset");
}

TEST_F(ConfigBuilderTest, FDv2_PollingFilterFlowsThrough) {
    ConfigBuilder builder("sdk-123");
    builder.DataSystem().Method(builders::DataSystemBuilder::FDv2().Polling(
        builders::FDv2Builder::PollingSource().Filter("flag-subset")));

    auto cfg = builder.Build();
    auto const fdv2_config =
        std::get<built::FDv2Config>(cfg->DataSystemConfig().system_);

    EXPECT_EQ(fdv2_config.polling.filter_key, "flag-subset");
}

TEST_F(ConfigBuilderTest, FDv2_DisableFDv1FallbackClearsIt) {
    ConfigBuilder builder("sdk-123");
    builder.DataSystem().Method(
        builders::DataSystemBuilder::FDv2().DisableFDv1Fallback());

    auto cfg = builder.Build();
    auto const fdv2_config =
        std::get<built::FDv2Config>(cfg->DataSystemConfig().system_);

    EXPECT_FALSE(fdv2_config.fdv1_fallback.has_value());
}

TEST_F(ConfigBuilderTest, FDv2_FallbackAndRecoveryTimeouts) {
    ConfigBuilder builder("sdk-123");
    builder.DataSystem().Method(builders::DataSystemBuilder::FDv2()
                                    .FallbackTimeout(std::chrono::seconds{30})
                                    .RecoveryTimeout(std::chrono::seconds{90}));

    auto cfg = builder.Build();
    auto const fdv2_config =
        std::get<built::FDv2Config>(cfg->DataSystemConfig().system_);

    EXPECT_EQ(fdv2_config.fallback_timeout, std::chrono::seconds{30});
    EXPECT_EQ(fdv2_config.recovery_timeout, std::chrono::seconds{90});
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

    // First establish that the data system is enabled,
    // to detect if defaults are misconfigured.
    auto const cfg1 = builder.Build();
    EXPECT_FALSE(cfg1->DataSystemConfig().disabled);

    builder.DataSystem().Disable();
    auto const cfg2 = builder.Build();
    EXPECT_TRUE(cfg2->DataSystemConfig().disabled);

    builder.DataSystem().Enabled(true);
    auto const cfg3 = builder.Build();
    EXPECT_FALSE(cfg3->DataSystemConfig().disabled);
}
