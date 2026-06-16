#include <gtest/gtest.h>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/logging/null_logger.hpp>
#include <launchdarkly/server_side/client.hpp>
#include <launchdarkly/server_side/config/config_builder.hpp>
#include <launchdarkly/server_side/integrations/big_segments/ibig_segment_store.hpp>

#include "config/builders/data_system/defaults.hpp"
#include "data_systems/background_sync/sources/streaming/streaming_data_source.hpp"

#include <chrono>

using namespace launchdarkly;
using namespace launchdarkly::server_side;
using namespace launchdarkly::server_side::config;

namespace {
// Minimal store so a non-null pointer can be threaded through the config.
class StubBigSegmentStore : public server_side::integrations::IBigSegmentStore {
   public:
    GetMembershipResult GetMembership(
        std::string const&) const noexcept override {
        return server_side::integrations::Membership::FromSegmentRefs({}, {});
    }
    GetMetadataResult GetMetadata() const noexcept override {
        return std::optional<server_side::integrations::StoreMetadata>{
            std::nullopt};
    }
};
}  // namespace

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

TEST_F(ConfigBuilderTest, BigSegmentsAbsentByDefault) {
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_TRUE(cfg);
    EXPECT_FALSE(cfg->BigSegments().has_value());
}

TEST_F(ConfigBuilderTest, BigSegmentsRoundTripsBuilderTunables) {
    using namespace std::chrono_literals;
    auto store = std::make_shared<StubBigSegmentStore>();

    ConfigBuilder builder("sdk-123");
    builder.BigSegments(builders::BigSegmentsBuilder(store)
                            .ContextCacheSize(50)
                            .ContextCacheTime(10s)
                            .StatusPollInterval(20s)
                            .StaleAfter(3min));

    auto cfg = builder.Build();
    ASSERT_TRUE(cfg);
    ASSERT_TRUE(cfg->BigSegments().has_value());
    EXPECT_EQ(cfg->BigSegments()->store, store);
    EXPECT_EQ(cfg->BigSegments()->context_cache_size, 50u);
    EXPECT_EQ(cfg->BigSegments()->context_cache_time, 10s);
    EXPECT_EQ(cfg->BigSegments()->status_poll_interval, 20s);
    EXPECT_EQ(cfg->BigSegments()->stale_after, 3min);
}

TEST_F(ConfigBuilderTest, BigSegmentsWithNullStoreFailsBuild) {
    ConfigBuilder builder("sdk-123");
    builder.BigSegments(builders::BigSegmentsBuilder(nullptr));

    auto cfg = builder.Build();
    ASSERT_FALSE(cfg);
    EXPECT_EQ(cfg.error(), Error::kConfig_BigSegments_NullStore);
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
