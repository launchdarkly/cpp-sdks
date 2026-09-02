#include <gtest/gtest.h>

#include <launchdarkly/client_side/bindings/c/config/builder.h>
#include <launchdarkly/client_side/bindings/c/config/fdv2_builder/fdv2_builder.h>
#include <launchdarkly/config/client.hpp>

#include <chrono>
#include <variant>

using launchdarkly::client_side::ConnectionMode;
using launchdarkly::config::shared::ClientSDK;

using FDv2Config = launchdarkly::config::shared::built::FDv2Config<ClientSDK>;

using namespace std::chrono_literals;

namespace {

// Builds a config through the C bindings and reads back the FDv2 settings.
FDv2Config BuildFDv2(LDClientFDv2Builder fdv2) {
    LDClientConfigBuilder builder = LDClientConfigBuilder_New("sdk-key");
    LDClientConfigBuilder_DataSource_MethodFDv2(builder, fdv2);

    LDClientConfig config = nullptr;
    LDStatus status = LDClientConfigBuilder_Build(builder, &config);
    EXPECT_TRUE(LDStatus_Ok(status));
    LDStatus_Free(status);

    auto const* built =
        reinterpret_cast<launchdarkly::client_side::Config*>(config);
    auto result = std::get<FDv2Config>(built->DataSourceConfig().method);
    LDClientConfig_Free(config);
    return result;
}

}  // namespace

TEST(FDv2Bindings, BuilderNewFree) {
    LDClientFDv2Builder builder = LDClientFDv2Builder_New();
    ASSERT_TRUE(builder);
    LDClientFDv2Builder_Free(builder);
}

TEST(FDv2Bindings, SourceBuilderNewFree) {
    LDClientFDv2StreamingBuilder streaming = LDClientFDv2StreamingBuilder_New();
    LDClientFDv2PollingBuilder polling = LDClientFDv2PollingBuilder_New();
    LDClientFDv2FDv1FallbackBuilder fallback =
        LDClientFDv2FDv1FallbackBuilder_New();
    LDClientFDv2ModeBuilder mode = LDClientFDv2ModeBuilder_New();

    ASSERT_TRUE(streaming);
    ASSERT_TRUE(polling);
    ASSERT_TRUE(fallback);
    ASSERT_TRUE(mode);

    LDClientFDv2StreamingBuilder_Free(streaming);
    LDClientFDv2PollingBuilder_Free(polling);
    LDClientFDv2FDv1FallbackBuilder_Free(fallback);
    LDClientFDv2ModeBuilder_Free(mode);
}

TEST(FDv2Bindings, SelectingFDv2ReplacesTheFDv1Method) {
    auto const config = BuildFDv2(LDClientFDv2Builder_New());

    EXPECT_EQ(ConnectionMode::kStreaming, config.initial_mode);
    EXPECT_EQ(3u, config.modes.size());
}

TEST(FDv2Bindings, InitialModeIsSelectable) {
    LDClientFDv2Builder builder = LDClientFDv2Builder_New();
    LDClientFDv2Builder_InitialMode(builder, LD_CLIENT_CONNECTION_MODE_OFFLINE);

    auto const config = BuildFDv2(builder);

    EXPECT_EQ(ConnectionMode::kOffline, config.initial_mode);
}

TEST(FDv2Bindings, UsePostIsSettable) {
    LDClientFDv2Builder builder = LDClientFDv2Builder_New();
    LDClientFDv2Builder_UsePost(builder, true);

    auto const config = BuildFDv2(builder);

    EXPECT_TRUE(config.use_post);
}

TEST(FDv2Bindings, CustomizingAModeReplacesItsPipeline) {
    LDClientFDv2StreamingBuilder streaming = LDClientFDv2StreamingBuilder_New();
    LDClientFDv2StreamingBuilder_InitialReconnectDelayMs(streaming, 2000);
    LDClientFDv2StreamingBuilder_BaseURL(streaming,
                                         "https://stream.example.com");

    LDClientFDv2ModeBuilder mode = LDClientFDv2ModeBuilder_New();
    LDClientFDv2ModeBuilder_Initializer_Cache(mode);
    LDClientFDv2ModeBuilder_Synchronizer_Streaming(mode, streaming);
    LDClientFDv2ModeBuilder_DisableFDv1Fallback(mode);

    LDClientFDv2Builder builder = LDClientFDv2Builder_New();
    LDClientFDv2Builder_CustomizeMode(
        builder, LD_CLIENT_CONNECTION_MODE_STREAMING, mode);

    auto const config = BuildFDv2(builder);
    auto const& definition = config.modes.at(ConnectionMode::kStreaming);

    ASSERT_EQ(1u, definition.initializers.size());
    EXPECT_TRUE(std::holds_alternative<FDv2Config::CacheConfig>(
        definition.initializers[0]));
    ASSERT_EQ(1u, definition.synchronizers.size());
    auto const* built =
        std::get_if<FDv2Config::StreamingConfig>(&definition.synchronizers[0]);
    ASSERT_NE(nullptr, built);
    EXPECT_EQ(2s, built->initial_reconnect_delay);
    ASSERT_TRUE(built->base_url_override.has_value());
    EXPECT_EQ("https://stream.example.com", *built->base_url_override);
    EXPECT_FALSE(definition.fdv1_fallback.has_value());
}

TEST(FDv2Bindings, PollingSourcesAreConfigurable) {
    LDClientFDv2PollingBuilder initializer = LDClientFDv2PollingBuilder_New();
    LDClientFDv2PollingBuilder_IntervalS(initializer, 60);

    LDClientFDv2PollingBuilder synchronizer = LDClientFDv2PollingBuilder_New();
    LDClientFDv2PollingBuilder_IntervalS(synchronizer, 120);
    LDClientFDv2PollingBuilder_BaseURL(synchronizer,
                                       "https://poll.example.com");

    LDClientFDv2ModeBuilder mode = LDClientFDv2ModeBuilder_New();
    LDClientFDv2ModeBuilder_Initializer_Polling(mode, initializer);
    LDClientFDv2ModeBuilder_Synchronizer_Polling(mode, synchronizer);

    LDClientFDv2Builder builder = LDClientFDv2Builder_New();
    LDClientFDv2Builder_CustomizeMode(builder,
                                      LD_CLIENT_CONNECTION_MODE_POLLING, mode);

    auto const config = BuildFDv2(builder);
    auto const& definition = config.modes.at(ConnectionMode::kPolling);

    ASSERT_EQ(1u, definition.initializers.size());
    auto const* first =
        std::get_if<FDv2Config::PollingConfig>(&definition.initializers[0]);
    ASSERT_NE(nullptr, first);
    EXPECT_EQ(60s, first->poll_interval);

    ASSERT_EQ(1u, definition.synchronizers.size());
    auto const* second =
        std::get_if<FDv2Config::PollingConfig>(&definition.synchronizers[0]);
    ASSERT_NE(nullptr, second);
    EXPECT_EQ(120s, second->poll_interval);
    ASSERT_TRUE(second->base_url_override.has_value());
    EXPECT_EQ("https://poll.example.com", *second->base_url_override);
}

TEST(FDv2Bindings, TheFDv1FallbackIsConfigurable) {
    LDClientFDv2FDv1FallbackBuilder fallback =
        LDClientFDv2FDv1FallbackBuilder_New();
    LDClientFDv2FDv1FallbackBuilder_IntervalS(fallback, 600);
    LDClientFDv2FDv1FallbackBuilder_BaseURL(fallback,
                                            "https://relay.example.com");

    LDClientFDv2ModeBuilder mode = LDClientFDv2ModeBuilder_New();
    LDClientFDv2ModeBuilder_Synchronizer_Streaming(
        mode, LDClientFDv2StreamingBuilder_New());
    LDClientFDv2ModeBuilder_FallbackToFDv1(mode, fallback);

    LDClientFDv2Builder builder = LDClientFDv2Builder_New();
    LDClientFDv2Builder_CustomizeMode(
        builder, LD_CLIENT_CONNECTION_MODE_STREAMING, mode);

    auto const config = BuildFDv2(builder);
    auto const& built =
        config.modes.at(ConnectionMode::kStreaming).fdv1_fallback;

    ASSERT_TRUE(built.has_value());
    EXPECT_EQ(600s, built->poll_interval);
    ASSERT_TRUE(built->base_url_override.has_value());
    EXPECT_EQ("https://relay.example.com", *built->base_url_override);
}
