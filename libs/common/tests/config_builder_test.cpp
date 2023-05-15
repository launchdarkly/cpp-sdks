#include <gtest/gtest.h>

#include <launchdarkly/config/client.hpp>
#include <launchdarkly/config/server.hpp>
#include "null_logger.hpp"

class ConfigBuilderTest
    : public ::testing::
          Test {  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
   protected:
    launchdarkly::Logger logger;
    ConfigBuilderTest() : logger(launchdarkly::logging::NullLogger()) {}
};

TEST_F(ConfigBuilderTest, DefaultConstruction_ClientConfig) {
    using namespace launchdarkly::client_side;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_TRUE(cfg);
    ASSERT_EQ(cfg->SdkKey(), "sdk-123");
}

TEST_F(ConfigBuilderTest, DefaultConstruction_ServerConfig) {
    using namespace launchdarkly::server_side;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_TRUE(cfg);
    ASSERT_EQ(cfg->SdkKey(), "sdk-123");
}

TEST_F(ConfigBuilderTest,
       DefaultConstruction_UsesDefaultEndpointsIfNotSupplied) {
    using namespace launchdarkly::client_side;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_EQ(cfg->ServiceEndpoints(), Defaults::ServiceEndpoints());
}

TEST_F(ConfigBuilderTest,
       DefaultConstruction_UsesDefaultOfflineModeIfNotSupplied) {
    using namespace launchdarkly::client_side;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_EQ(cfg->Offline(), Defaults::Offline());
}

// This test should exercise all of the config options.
TEST_F(ConfigBuilderTest, CustomBuilderReflectsChanges) {
    using namespace launchdarkly::client_side;
    auto config =
        ConfigBuilder("sdk-123")
            .Offline(true)
            .ServiceEndpoints(EndpointsBuilder().RelayProxy("foo"))
            .AppInfo(AppInfoBuilder().Identifier("bar").Version("baz"))
            .Build();

    ASSERT_TRUE(config);
    ASSERT_EQ(config->SdkKey(), "sdk-123");
    ASSERT_TRUE(config->Offline());
    ASSERT_EQ(config->ServiceEndpoints(),
              EndpointsBuilder().RelayProxy("foo").Build());
    ASSERT_EQ(config->ApplicationTag(),
              "application-id/bar application-version/baz");
}

TEST_F(ConfigBuilderTest,
       DefaultConstruction_ClientConfig_UsesDefaulDataSourceConfig) {
    using namespace launchdarkly::client_side;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();

    EXPECT_FALSE(cfg->DataSourceConfig().with_reasons);
    EXPECT_FALSE(cfg->DataSourceConfig().use_report);
    // Should be streaming with a 1 second initial retry;
    EXPECT_EQ(
        std::chrono::milliseconds{1000},
        std::get<launchdarkly::config::shared::built::StreamingConfig<
            launchdarkly::config::ClientSDK>>(cfg->DataSourceConfig().method)
            .initial_reconnect_delay);
}

TEST_F(ConfigBuilderTest,
       DefaultConstruction_ServerConfig_UsesDefaulDataSourceConfig) {
    using namespace launchdarkly::server_side;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();

    // Should be streaming with a 1 second initial retry;
    EXPECT_EQ(
        std::chrono::milliseconds{1000},
        std::get<launchdarkly::config::shared::built::StreamingConfig<
            launchdarkly::config::ServerSDK>>(cfg->DataSourceConfig().method)
            .initial_reconnect_delay);
}

TEST_F(ConfigBuilderTest, ServerConfig_CanSetDataSource) {
    using namespace launchdarkly::server_side;
    ConfigBuilder builder("sdk-123");

    builder.DataSource(ConfigBuilder::DataSourceBuilder().Method(
        ConfigBuilder::DataSourceBuilder::Streaming().InitialReconnectDelay(
            std::chrono::milliseconds{5000})));

    auto cfg = builder.Build();

    EXPECT_EQ(
        std::chrono::milliseconds{5000},
        std::get<launchdarkly::config::shared::built::StreamingConfig<
            launchdarkly::config::ServerSDK>>(cfg->DataSourceConfig().method)
            .initial_reconnect_delay);
}

TEST_F(ConfigBuilderTest, ClientConfig_CanSetDataSource) {
    using namespace launchdarkly::client_side;
    ConfigBuilder builder("sdk-123");

    builder.DataSource(
        ConfigBuilder::DataSourceBuilder()
            .Method(ConfigBuilder::DataSourceBuilder::Streaming()
                        .InitialReconnectDelay(std::chrono::milliseconds{5000}))
            .UseReport(true)
            .WithReasons(true));

    auto cfg = builder.Build();
    ASSERT_TRUE(cfg);

    EXPECT_TRUE(cfg->DataSourceConfig().use_report);
    EXPECT_TRUE(cfg->DataSourceConfig().with_reasons);
    EXPECT_EQ(
        std::chrono::milliseconds{5000},
        std::get<launchdarkly::config::shared::built::StreamingConfig<
            launchdarkly::config::ClientSDK>>(cfg->DataSourceConfig().method)
            .initial_reconnect_delay);
}

TEST_F(ConfigBuilderTest,
       DefaultConstruction_ClientConfig_UsesDefaultHttpProperties) {
    using namespace launchdarkly::client_side;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();

    ASSERT_EQ(cfg->HttpProperties(), Defaults::HttpProperties());
}

TEST_F(ConfigBuilderTest,
       DefaultConstruction_ServerConfig_UsesDefaultHttpProperties) {
    using namespace launchdarkly::server_side;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_EQ(cfg->HttpProperties(), Defaults::HttpProperties());
}

TEST_F(ConfigBuilderTest, DefaultConstruction_CanSetHttpProperties) {
    using namespace launchdarkly::client_side;
    ConfigBuilder builder("sdk-123");
    builder.HttpProperties(
        ConfigBuilder::HttpPropertiesBuilder()
            .ConnectTimeout(std::chrono::milliseconds{1234})
            .ReadTimeout(std::chrono::milliseconds{123456})
            .WrapperName("potato")
            .WrapperVersion("2.0-chip")
            .CustomHeaders(
                std::map<std::string, std::string>{{"color", "green"}}));

    auto cfg = builder.Build();
    ASSERT_TRUE(cfg);

    EXPECT_EQ("CppClient/TODO", cfg->HttpProperties().UserAgent());
    EXPECT_EQ(123456, cfg->HttpProperties().ReadTimeout().count());
    EXPECT_EQ(1234, cfg->HttpProperties().ConnectTimeout().count());
    EXPECT_EQ("potato/2.0-chip",
              cfg->HttpProperties().BaseHeaders().at("X-LaunchDarkly-Wrapper"));
    EXPECT_EQ("green", cfg->HttpProperties().BaseHeaders().at("color"));
}
