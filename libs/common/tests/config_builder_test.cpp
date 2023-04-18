#include <gtest/gtest.h>

#include "config/client.hpp"
#include "config/server.hpp"
#include "null_logger.hpp"

class ConfigBuilderTest
    : public ::testing::
          Test {  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
   protected:
    launchdarkly::Logger logger;
    ConfigBuilderTest() : logger(NullLogger()) {}
};

TEST_F(ConfigBuilderTest, DefaultConstruction_ClientConfig) {
    using namespace launchdarkly::client;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_TRUE(cfg);
    ASSERT_EQ(cfg->sdk_key(), "sdk-123");
}

TEST_F(ConfigBuilderTest, DefaultConstruction_ServerConfig) {
    using namespace launchdarkly::server;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_TRUE(cfg);
    ASSERT_EQ(cfg->sdk_key(), "sdk-123");
}

TEST_F(ConfigBuilderTest,
       DefaultConstruction_UsesDefaultEndpointsIfNotSupplied) {
    using namespace launchdarkly::client;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_EQ(cfg->service_endpoints(), Defaults::EndpointsConfig());
}

TEST_F(ConfigBuilderTest,
       DefaultConstruction_UsesDefaultOfflineModeIfNotSupplied) {
    using namespace launchdarkly::client;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_EQ(cfg->offline(), Defaults::Offline());
}

// This test should exercise all of the config options.
TEST_F(ConfigBuilderTest, CustomBuilderReflectsChanges) {
    using namespace launchdarkly::client;
    auto config =
        ConfigBuilder("sdk-123")
            .Offline(true)
            .ServiceEndpoints(EndpointsBuilder().RelayProxy("foo"))
            .AppInfo(AppInfoBuilder().Identifier("bar").Version("baz"))
            .Build();

    ASSERT_TRUE(config);
    ASSERT_EQ(config->sdk_key(), "sdk-123");
    ASSERT_TRUE(config->offline());
    ASSERT_EQ(config->service_endpoints(),
              EndpointsBuilder().RelayProxy("foo").Build());
    ASSERT_EQ(config->application_tag(),
              "application-id/bar application-version/baz");
}

TEST_F(ConfigBuilderTest,
       DefaultConstruction_ClientConfig_UsesDefaulDataSourceConfig) {
    using namespace launchdarkly::client;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();

    EXPECT_FALSE(cfg->data_source_config().with_reasons);
    EXPECT_FALSE(cfg->data_source_config().use_report);
    // Should be streaming with a 1 second initial retry;
    EXPECT_EQ(std::chrono::milliseconds{1000},
              boost::get<launchdarkly::config::detail::StreamingConfig>(
                  cfg->data_source_config().method)
                  .initial_reconnect_delay);
}

TEST_F(ConfigBuilderTest,
       DefaultConstruction_ServerConfig_UsesDefaulDataSourceConfig) {
    using namespace launchdarkly::server;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();

    // Should be streaming with a 1 second initial retry;
    EXPECT_EQ(std::chrono::milliseconds{1000},
              boost::get<launchdarkly::config::detail::StreamingConfig>(
                  cfg->data_source_config().method)
                  .initial_reconnect_delay);
}

TEST_F(ConfigBuilderTest, ServerConfig_CanSetDataSource) {
    using namespace launchdarkly::server;
    ConfigBuilder builder("sdk-123");

    builder.DataSource(ConfigBuilder::DataSourceBuilder().method(
        ConfigBuilder::DataSourceBuilder::Streaming().InitialReconnectDelay(
            std::chrono::milliseconds{5000})));

    auto cfg = builder.Build();

    EXPECT_EQ(std::chrono::milliseconds{5000},
              boost::get<launchdarkly::config::detail::StreamingConfig>(
                  cfg->data_source_config().method)
                  .initial_reconnect_delay);
}

TEST_F(ConfigBuilderTest, ClientConfig_CanSetDataSource) {
    using namespace launchdarkly::client;
    ConfigBuilder builder("sdk-123");

    builder.DataSource(
        ConfigBuilder::DataSourceBuilder()
            .method(ConfigBuilder::DataSourceBuilder::Streaming()
                        .InitialReconnectDelay(std::chrono::milliseconds{5000}))
            .use_report(true)
            .with_reasons(true));

    auto cfg = builder.Build();
    ASSERT_TRUE(cfg);

    EXPECT_TRUE(cfg->data_source_config().use_report);
    EXPECT_TRUE(cfg->data_source_config().with_reasons);
    EXPECT_EQ(std::chrono::milliseconds{5000},
              boost::get<launchdarkly::config::detail::StreamingConfig>(
                  cfg->data_source_config().method)
                  .initial_reconnect_delay);
}

TEST_F(ConfigBuilderTest,
       DefaultConstruction_ClientConfig_UsesDefaultHttpProperties) {
    using namespace launchdarkly::client;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();

    ASSERT_EQ(cfg->http_properties(), Defaults::HttpConfig());
}

TEST_F(ConfigBuilderTest,
       DefaultConstruction_ServerConfig_UsesDefaultHttpProperties) {
    using namespace launchdarkly::server;
    ConfigBuilder builder("sdk-123");
    auto cfg = builder.Build();
    ASSERT_EQ(cfg->http_properties(), Defaults::HttpConfig());
}

TEST_F(ConfigBuilderTest, DefaultConstruction_CanSetHttpProperties) {
    using namespace launchdarkly::client;
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

    EXPECT_EQ("CppClient/TODO", cfg->http_properties().UserAgent());
    EXPECT_EQ(123456, cfg->http_properties().ReadTimeout().count());
    EXPECT_EQ(1234, cfg->http_properties().ConnectTimeout().count());
    EXPECT_EQ("potato/2.0-chip", cfg->http_properties().BaseHeaders().at(
                                     "X-LaunchDarkly-Wrapper"));
    EXPECT_EQ("green", cfg->http_properties().BaseHeaders().at("color"));
}
