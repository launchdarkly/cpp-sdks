#include <gtest/gtest.h>

#include "config/client_endpoints_builder.hpp"
#include "config/server_endpoints_builder.hpp"

using namespace launchdarkly::config;

class ServiceEndpointTest : public testing::Test {};

TEST(ServiceEndpointTest, DefaultClientBuilderURLs) {
    ClientEndpointsBuilder b;
    std::unique_ptr<ServiceEndpoints> eps = b.build();
    ASSERT_TRUE(eps);
    ASSERT_EQ(eps->polling_base_url(), "https://clientsdk.launchdarkly.com");
    ASSERT_EQ(eps->streaming_base_url(),
              "https://clientstream.launchdarkly.com");
    ASSERT_EQ(eps->events_base_url(), "https://mobile.launchdarkly.com");
}

TEST(ServiceEndpointTest, DefaultServerBuilderURLs) {
    ServerEndpointsBuilder b;
    std::unique_ptr<ServiceEndpoints> eps = b.build();
    ASSERT_TRUE(eps);
    ASSERT_EQ(eps->polling_base_url(), "https://sdk.launchdarkly.com");
    ASSERT_EQ(eps->streaming_base_url(), "https://stream.launchdarkly.com");
    ASSERT_EQ(eps->events_base_url(), "https://events.launchdarkly.com");
}

TEST(ServiceEndpointTest, ModifySingleURLCausesError_Polling) {
    ClientEndpointsBuilder b;
    b.polling_base_url("foo");
    ASSERT_FALSE(b.build());
}

TEST(ServiceEndpointTest, ModifySingleURLCausesError_Streaming) {
    ClientEndpointsBuilder b;
    b.streaming_base_url("foo");
    ASSERT_FALSE(b.build());
}

TEST(ServiceEndpointTest, ModifySingleURLCausesError_Events) {
    ClientEndpointsBuilder b;
    b.events_base_url("foo");
    ASSERT_FALSE(b.build());
}

TEST(ServiceEndpointsTest, RelaySetsAllURLS) {
    ClientEndpointsBuilder b;
    b.relay_proxy("foo");
    std::unique_ptr<ServiceEndpoints> eps = b.build();
    ASSERT_TRUE(eps);
    ASSERT_EQ(eps->streaming_base_url(), "foo");
    ASSERT_EQ(eps->polling_base_url(), "foo");
    ASSERT_EQ(eps->events_base_url(), "foo");
}

TEST(ServiceEndpointsTest, TrimsTrailingSlashes) {
    {
        ClientEndpointsBuilder b;
        b.relay_proxy("foo/");
        auto eps = b.build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->streaming_base_url(), "foo");
    }

    {
        ClientEndpointsBuilder b;
        b.relay_proxy("foo////////");
        auto eps = b.build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->streaming_base_url(), "foo");
    }

    {
        ClientEndpointsBuilder b;
        b.relay_proxy("/");
        auto eps = b.build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->streaming_base_url(), "");
    }
}

TEST(ServiceEndpointsTest, EmptyURLsAreInvalid) {
    {
        ClientEndpointsBuilder b;
        b.relay_proxy("");
        auto eps = b.build();
        ASSERT_FALSE(eps);
    }
    {
        ClientEndpointsBuilder b;
        auto eps = b.streaming_base_url("")
                       .events_base_url("foo")
                       .polling_base_url("bar")
                       .build();
        ASSERT_FALSE(eps);
    }

    {
        ClientEndpointsBuilder b;
        auto eps = b.streaming_base_url("foo")
                       .events_base_url("bar")
                       .polling_base_url("")
                       .build();
        ASSERT_FALSE(eps);
    }
}
