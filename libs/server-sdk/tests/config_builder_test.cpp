#include <gtest/gtest.h>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/logging/null_logger.hpp>
#include <launchdarkly/server_side/client.hpp>
#include <launchdarkly/server_side/config/config_builder.hpp>

#include "data_systems/background_sync/sources/streaming/streaming_data_source.hpp"

using namespace launchdarkly;
using namespace launchdarkly::server_side;
using namespace launchdarkly::config::shared;

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

    ASSERT_TRUE(std::holds_alternative<built::BackgroundSyncConfig<ServerSDK>>(
        cfg->DataSystemConfig().system_));

    auto const bg_sync_config =
        std::get<built::BackgroundSyncConfig<ServerSDK>>(
            cfg->DataSystemConfig().system_);

    ASSERT_TRUE(std::holds_alternative<built::StreamingConfig<ServerSDK>>(
        bg_sync_config.source_.method));

    auto const streaming_config = std::get<built::StreamingConfig<ServerSDK>>(
        bg_sync_config.source_.method);

    EXPECT_EQ(streaming_config, server_side::Defaults::StreamingConfig());
}

TEST_F(ConfigBuilderTest, DefaultConstruction_HttpPropertyDefaultsAreUsed) {
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_EQ(cfg->HttpProperties(), server_side::Defaults::HttpProperties());
}

TEST_F(ConfigBuilderTest, DefaultConstruction_ServiceEndpointDefaultsAreUsed) {
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_EQ(cfg->ServiceEndpoints(),
              server_side::Defaults::ServiceEndpoints());
}

TEST_F(ConfigBuilderTest, DefaultConstruction_EventDefaultsAreUsed) {
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_EQ(cfg->Events(), server_side::Defaults::Events());
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

TEST_F(ConfigBuilderTest, CanConstructValidRedisConfig) {
    ConfigBuilder builder("sdk-123");

    using LazyLoad = DataSystemBuilder::LazyLoad;

    builder.DataSystem().Method(
        LazyLoad()
            .Source(LazyLoad::Redis().Connection("tcp://localhost:1234"))
            .CacheEviction(LazyLoad::EvictionPolicy::Disabled)
            .CacheTTL(std::chrono::seconds(5)));

    auto cfg = builder.Build();
    ASSERT_TRUE(cfg);
}

TEST_F(ConfigBuilderTest, InvalidRedisConfigurationDetected) {
    ConfigBuilder builder("sdk-123");

    using LazyLoad = DataSystemBuilder::LazyLoad;

    // An empty URI string should be rejected before it is passed deeper
    // into the redis client.
    builder.DataSystem().Method(
        LazyLoad().Source(LazyLoad::Redis().Connection("")));

    auto cfg = builder.Build();
    ASSERT_EQ(cfg.error(), Error::kConfig_DataSource_Redis_MissingURI);

    // If using ConnOpts instead of a URI string, the host should be rejected
    // for same reason as above.
    builder.DataSystem().Method(LazyLoad().Source(LazyLoad::Redis().Connection(
        LazyLoad::Redis::ConnOpts{"", 1233, "password", 2})));

    cfg = builder.Build();
    ASSERT_EQ(cfg.error(), Error::kConfig_DataSource_Redis_MissingHost);

    // If the port isn't set, it'll be default-constructed as std::nullopt.
    builder.DataSystem().Method(LazyLoad().Source(LazyLoad::Redis().Connection(
        LazyLoad::Redis::ConnOpts{"tcp://localhost"})));

    cfg = builder.Build();
    ASSERT_EQ(cfg.error(), Error::kConfig_DataSource_Redis_MissingPort);
}
