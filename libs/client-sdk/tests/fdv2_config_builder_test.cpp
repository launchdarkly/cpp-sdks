#include <gtest/gtest.h>

#include <launchdarkly/config/client.hpp>

#include <chrono>
#include <variant>

using launchdarkly::client_side::ConfigBuilder;
using launchdarkly::client_side::ConnectionMode;
using launchdarkly::client_side::DataSourceBuilder;
using launchdarkly::client_side::Defaults;
using launchdarkly::client_side::FDv2Builder;
using launchdarkly::config::shared::ClientSDK;

using FDv2Config = launchdarkly::config::shared::built::FDv2Config<ClientSDK>;
using PollingConfig =
    launchdarkly::config::shared::built::PollingConfig<ClientSDK>;
using StreamingConfig =
    launchdarkly::config::shared::built::StreamingConfig<ClientSDK>;

using namespace std::chrono_literals;

namespace {

FDv2Config BuildFDv2(FDv2Builder fdv2) {
    auto config =
        ConfigBuilder("sdk-key").DataSource().Method(std::move(fdv2)).Build();
    return std::get<FDv2Config>(config.method);
}

}  // namespace

// The streaming and polling methods speak FDv1. FDv2 replaces them.
TEST(FDv2ConfigBuilderTest, TheDefaultDataSourceIsStillFDv1Streaming) {
    auto config = ConfigBuilder("sdk-key").Build();

    ASSERT_TRUE(config.has_value());
    EXPECT_TRUE(std::holds_alternative<StreamingConfig>(
        config->DataSourceConfig().method));
}

TEST(FDv2ConfigBuilderTest, SelectingFDv2ReplacesTheFDv1Method) {
    auto builder = ConfigBuilder("sdk-key");
    builder.DataSource().Method(DataSourceBuilder::FDv2());

    auto config = builder.Build();

    ASSERT_TRUE(config.has_value());
    EXPECT_TRUE(
        std::holds_alternative<FDv2Config>(config->DataSourceConfig().method));
}

// What those defaults are is pinned down where they are defined. This is
// about the builder starting from them.
TEST(FDv2ConfigBuilderTest, AnUncustomizedBuilderYieldsTheSDKDefaults) {
    auto const config = BuildFDv2(FDv2Builder());
    auto const defaults = Defaults::FDv2Config();

    EXPECT_EQ(defaults.initial_mode, config.initial_mode);
    EXPECT_EQ(defaults.modes.size(), config.modes.size());
    EXPECT_EQ(defaults.fallback_timeout, config.fallback_timeout);
    EXPECT_EQ(defaults.recovery_timeout, config.recovery_timeout);
}

TEST(FDv2ConfigBuilderTest, DefaultsSendTheContextInTheRequestPath) {
    auto const config = BuildFDv2(FDv2Builder());

    EXPECT_FALSE(config.use_post);
}

TEST(FDv2ConfigBuilderTest, InitialModeIsSelectable) {
    auto const config =
        BuildFDv2(FDv2Builder().InitialMode(ConnectionMode::kPolling));

    EXPECT_EQ(ConnectionMode::kPolling, config.initial_mode);
}

TEST(FDv2ConfigBuilderTest, UsePostIsSettable) {
    auto const config = BuildFDv2(FDv2Builder().UsePost(true));

    EXPECT_TRUE(config.use_post);
}

TEST(FDv2ConfigBuilderTest, CustomizingAModeReplacesItsPipeline) {
    FDv2Builder builder;
    builder.CustomizeMode(
        ConnectionMode::kStreaming,
        FDv2Builder::Mode()
            .Initializer(FDv2Builder::Cache())
            .Synchronizer(FDv2Builder::Streaming().InitialReconnectDelay(2s))
            .DisableFDv1Fallback());

    auto const config = BuildFDv2(std::move(builder));
    auto const& mode = config.modes.at(ConnectionMode::kStreaming);

    ASSERT_EQ(1u, mode.initializers.size());
    EXPECT_TRUE(
        std::holds_alternative<FDv2Config::CacheConfig>(mode.initializers[0]));
    ASSERT_EQ(1u, mode.synchronizers.size());
    auto const* streaming =
        std::get_if<FDv2Config::StreamingConfig>(&mode.synchronizers[0]);
    ASSERT_NE(nullptr, streaming);
    EXPECT_EQ(2s, streaming->initial_reconnect_delay);
    EXPECT_FALSE(mode.fdv1_fallback.has_value());
}

TEST(FDv2ConfigBuilderTest, CustomizingOneModeLeavesTheOthersAlone) {
    FDv2Builder builder;
    builder.CustomizeMode(ConnectionMode::kPolling,
                          FDv2Builder::Mode().Synchronizer(
                              FDv2Builder::Polling().PollInterval(60s)));

    auto const config = BuildFDv2(std::move(builder));

    EXPECT_EQ(3u, config.modes.size());
    EXPECT_EQ(2u,
              config.modes.at(ConnectionMode::kStreaming).initializers.size());
}

TEST(FDv2ConfigBuilderTest, SourcesCanOverrideTheirOwnBaseUrl) {
    FDv2Builder builder;
    builder.CustomizeMode(
        ConnectionMode::kStreaming,
        FDv2Builder::Mode().Synchronizer(
            FDv2Builder::Polling().BaseUrl("https://relay.example.com")));

    auto const config = BuildFDv2(std::move(builder));
    auto const* polling = std::get_if<FDv2Config::PollingConfig>(
        &config.modes.at(ConnectionMode::kStreaming).synchronizers[0]);

    ASSERT_NE(nullptr, polling);
    ASSERT_TRUE(polling->base_url_override.has_value());
    EXPECT_EQ("https://relay.example.com", *polling->base_url_override);
}

TEST(FDv2ConfigBuilderTest, TheFDv1FallbackIsConfigurable) {
    FDv2Builder builder;
    builder.CustomizeMode(
        ConnectionMode::kStreaming,
        FDv2Builder::Mode()
            .Synchronizer(FDv2Builder::Streaming())
            .FallbackToFDv1(
                FDv2Builder::FDv1Fallback().PollInterval(600s).BaseUrl(
                    "https://relay.example.com")));

    auto const config = BuildFDv2(std::move(builder));
    auto const& fallback =
        config.modes.at(ConnectionMode::kStreaming).fdv1_fallback;

    ASSERT_TRUE(fallback.has_value());
    EXPECT_EQ(600s, fallback->poll_interval);
    ASSERT_TRUE(fallback->base_url_override.has_value());
    EXPECT_EQ("https://relay.example.com", *fallback->base_url_override);
}

// ============================================================================
// Endpoint defaults
// ============================================================================

namespace {

FDv2Config BuiltFDv2(ConfigBuilder& builder) {
    auto config = builder.Build();
    EXPECT_TRUE(config.has_value());
    return std::get<FDv2Config>(config->DataSourceConfig().method);
}

}  // namespace

// FDv2 polls a different endpoint than FDv1 does, so an application that
// configured nothing polls there rather than where FDv1 would.
TEST(FDv2ConfigBuilderTest, FDv2KeepsItsOwnDefaultEndpoints) {
    auto builder = ConfigBuilder("sdk-key");
    builder.DataSource().Method(DataSourceBuilder::FDv2());

    auto const fdv2 = BuiltFDv2(builder);

    EXPECT_EQ("https://sdk.launchdarkly.com", fdv2.polling_base_url);
    EXPECT_EQ("https://clientstream.launchdarkly.com", fdv2.streaming_base_url);
    EXPECT_NE(Defaults::ServiceEndpoints().PollingBaseUrl(),
              fdv2.polling_base_url);
}

// An application that pointed the SDK somewhere else — a Relay Proxy, say —
// has FDv2 follow it there.
TEST(FDv2ConfigBuilderTest, ConfiguredEndpointsRedirectFDv2) {
    auto builder = ConfigBuilder("sdk-key");
    builder.DataSource().Method(DataSourceBuilder::FDv2());
    builder.ServiceEndpoints().RelayProxyBaseURL("https://relay.example.com");

    auto const fdv2 = BuiltFDv2(builder);

    EXPECT_EQ("https://relay.example.com", fdv2.polling_base_url);
    EXPECT_EQ("https://relay.example.com", fdv2.streaming_base_url);
}

// Endpoints set to the SDK's own values are still the application's choice,
// and redirect FDv2 like any other.
TEST(FDv2ConfigBuilderTest, ConfiguringOnlyTheEventsUrlStillRedirectsFDv2) {
    auto defaults = Defaults::ServiceEndpoints();
    auto builder = ConfigBuilder("sdk-key");
    builder.DataSource().Method(DataSourceBuilder::FDv2());
    builder.ServiceEndpoints()
        .EventsBaseUrl("https://events.example.com")
        .PollingBaseUrl(defaults.PollingBaseUrl())
        .StreamingBaseUrl(defaults.StreamingBaseUrl());

    auto const fdv2 = BuiltFDv2(builder);

    EXPECT_EQ(defaults.PollingBaseUrl(), fdv2.polling_base_url);
}
