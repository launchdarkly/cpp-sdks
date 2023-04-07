#include <gtest/gtest.h>

#include "config/client.hpp"
#include "config/server.hpp"
#include "error.hpp"

class ServiceEndpointTest : public testing::Test {};

using ClientEndpointsBuilder = launchdarkly::client::Endpoints;

using ServerEndpointsBuilder = launchdarkly::server::Endpoints;

using launchdarkly::Error;

TEST(ServiceEndpointTest, DefaultClientBuilderURLs) {
    ClientEndpointsBuilder builder;
    auto eps = builder.build();
    ASSERT_TRUE(eps);
    ASSERT_EQ(eps->polling_host(), "https://clientsdk.launchdarkly.com");
    ASSERT_EQ(eps->streaming_host(), "https://clientstream.launchdarkly.com");
    ASSERT_EQ(eps->events_host(), "https://mobile.launchdarkly.com");
}

TEST(ServiceEndpointTest, DefaultServerBuilderURLs) {
    ServerEndpointsBuilder builder;
    auto eps = builder.build();
    ASSERT_TRUE(eps);
    ASSERT_EQ(eps->polling_host(), "https://sdk.launchdarkly.com");
    ASSERT_EQ(eps->streaming_host(), "https://stream.launchdarkly.com");
    ASSERT_EQ(eps->events_host(), "https://events.launchdarkly.com");
}

TEST(ServiceEndpointTest, ModifySingleURLCausesError) {
    auto result = ClientEndpointsBuilder().polling_host("foo").build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_AllURLsMustBeSet);

    result = ClientEndpointsBuilder().streaming_host("foo").build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_AllURLsMustBeSet);

    result = ClientEndpointsBuilder().events_host("foo").build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_AllURLsMustBeSet);
}

TEST(ServiceEndpointsTest, RelaySetsAllURLS) {
    auto eps = ClientEndpointsBuilder().relay_proxy("foo").build();
    ASSERT_TRUE(eps);
    ASSERT_EQ(eps->streaming_host(), "foo");
    ASSERT_EQ(eps->polling_host(), "foo");
    ASSERT_EQ(eps->events_host(), "foo");
}

TEST(ServiceEndpointsTest, TrimsTrailingSlashes) {
    {
        auto eps = ClientEndpointsBuilder().relay_proxy("foo/").build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->streaming_host(), "foo");
    }

    {
        auto eps = ClientEndpointsBuilder().relay_proxy("foo////////").build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->streaming_host(), "foo");
    }

    {
        auto eps = ClientEndpointsBuilder().relay_proxy("/").build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->streaming_host(), "");
    }
}

TEST(ServiceEndpointsTest, EmptyURLsAreInvalid) {
    auto result = ClientEndpointsBuilder().relay_proxy("").build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_EmptyURL);

    result = ClientEndpointsBuilder()
                 .streaming_host("")
                 .events_host("foo")
                 .polling_host("bar")
                 .build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_EmptyURL);

    result = ClientEndpointsBuilder()
                 .streaming_host("foo")
                 .events_host("")
                 .polling_host("bar")
                 .build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_EmptyURL);

    result = ClientEndpointsBuilder()
                 .streaming_host("foo")
                 .events_host("bar")
                 .polling_host("")
                 .build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_EmptyURL);
}
