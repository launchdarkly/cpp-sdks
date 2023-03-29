#include <gtest/gtest.h>

#include "config/client.hpp"
#include "config/server.hpp"

class ServiceEndpointTest : public testing::Test {};

using ClientEndpointsBuilder =
    launchdarkly::client::ConfigBuilder::EndpointsBuilder;

using ServerEndpointsBuilder =
    launchdarkly::server::ConfigBuilder::EndpointsBuilder;

TEST(ServiceEndpointTest, DefaultClientBuilderURLs) {
    ClientEndpointsBuilder builder;
    auto eps = builder.build();
    ASSERT_TRUE(eps);
    ASSERT_EQ(eps->polling_base_url(), "https://clientsdk.launchdarkly.com");
    ASSERT_EQ(eps->streaming_base_url(),
              "https://clientstream.launchdarkly.com");
    ASSERT_EQ(eps->events_base_url(), "https://mobile.launchdarkly.com");
}

TEST(ServiceEndpointTest, DefaultServerBuilderURLs) {
    launchdarkly::server::ConfigBuilder::EndpointsBuilder builder;
    auto eps = builder.build();
    ASSERT_TRUE(eps);
    ASSERT_EQ(eps->polling_base_url(), "https://sdk.launchdarkly.com");
    ASSERT_EQ(eps->streaming_base_url(), "https://stream.launchdarkly.com");
    ASSERT_EQ(eps->events_base_url(), "https://events.launchdarkly.com");
}

TEST(ServiceEndpointTest, ModifySingleURLCausesError_Polling) {
    ClientEndpointsBuilder builder;
    builder.polling_base_url("foo");
    ASSERT_FALSE(builder.build());
}

TEST(ServiceEndpointTest, ModifySingleURLCausesError_Streaming) {
    ClientEndpointsBuilder builder;
    builder.streaming_base_url("foo");
    ASSERT_FALSE(builder.build());
}

TEST(ServiceEndpointTest, ModifySingleURLCausesError_Events) {
    ClientEndpointsBuilder builder;
    builder.events_base_url("foo");
    ASSERT_FALSE(builder.build());
}

TEST(ServiceEndpointsTest, RelaySetsAllURLS) {
    ClientEndpointsBuilder builder;
    builder.relay_proxy("foo");
    auto eps = builder.build();
    ASSERT_TRUE(eps);
    ASSERT_EQ(eps->streaming_base_url(), "foo");
    ASSERT_EQ(eps->polling_base_url(), "foo");
    ASSERT_EQ(eps->events_base_url(), "foo");
}

TEST(ServiceEndpointsTest, TrimsTrailingSlashes) {
    {
        ClientEndpointsBuilder builder;
        builder.relay_proxy("foo/");
        auto eps = builder.build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->streaming_base_url(), "foo");
    }

    {
        ClientEndpointsBuilder builder;
        builder.relay_proxy("foo////////");
        auto eps = builder.build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->streaming_base_url(), "foo");
    }

    {
        ClientEndpointsBuilder builder;
        builder.relay_proxy("/");
        auto eps = builder.build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->streaming_base_url(), "");
    }
}

TEST(ServiceEndpointsTest, EmptyURLsAreInvalid) {
    {
        ClientEndpointsBuilder builder;
        builder.relay_proxy("");
        auto eps = builder.build();
        ASSERT_FALSE(eps);
    }
    {
        ClientEndpointsBuilder builder;
        auto eps = builder.streaming_base_url("")
                       .events_base_url("foo")
                       .polling_base_url("bar")
                       .build();
        ASSERT_FALSE(eps);
    }

    {
        ClientEndpointsBuilder builder;
        auto eps = builder.streaming_base_url("foo")
                       .events_base_url("bar")
                       .polling_base_url("")
                       .build();
        ASSERT_FALSE(eps);
    }
}
