#include <gtest/gtest.h>
#include "config/client.hpp"
#include "config/server.hpp"
#include "value.hpp"

#include <boost/json.hpp>

using namespace launchdarkly;

TEST(DataSourceBuilderTests, CanCreateStreamingClientConfig) {
    auto client_config =
        client_side::DataSourceBuilder()
            .with_reasons(true)
            .use_report(true)
            .method(client_side::DataSourceBuilder::Streaming()
                        .InitialReconnectDelay(std::chrono::milliseconds{1500}))
            .Build();

    EXPECT_TRUE(client_config.use_report);
    EXPECT_TRUE(client_config.with_reasons);
    EXPECT_EQ(
        std::chrono::milliseconds{1500},
        boost::get<config::detail::built::StreamingConfig>(client_config.method)
            .initial_reconnect_delay);
}

TEST(DataSourceBuilderTests, CanCreatePollingClientConfig) {
    auto client_config =
        client_side::DataSourceBuilder()
            .with_reasons(false)
            .use_report(false)
            .method(client_side::DataSourceBuilder::Polling().poll_interval(
                std::chrono::seconds{88000}))
            .Build();

    EXPECT_FALSE(client_config.use_report);
    EXPECT_FALSE(client_config.with_reasons);
    EXPECT_EQ(
        std::chrono::seconds{88000},
        boost::get<config::detail::built::PollingConfig>(client_config.method)
            .poll_interval);
}

TEST(DataSourceBuilderTests, CanCreateStreamingServerConfig) {
    auto server_config =
        server_side::DataSourceBuilder()
            .method(server_side::DataSourceBuilder::Streaming()
                        .InitialReconnectDelay(std::chrono::milliseconds{1500}))
            .Build();

    EXPECT_EQ(
        std::chrono::milliseconds{1500},
        boost::get<config::detail::built::StreamingConfig>(server_config.method)
            .initial_reconnect_delay);
}

TEST(DataSourceBuilderTests, CanCreatePollingServerConfig) {
    auto server_config =
        server_side::DataSourceBuilder()
            .method(server_side::DataSourceBuilder::Polling().poll_interval(
                std::chrono::seconds{30000}))
            .Build();

    EXPECT_EQ(
        std::chrono::seconds{30000},
        boost::get<config::detail::built::PollingConfig>(server_config.method)
            .poll_interval);
}
