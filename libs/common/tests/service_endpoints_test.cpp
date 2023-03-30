#include <gtest/gtest.h>

#include "config/client.hpp"
#include "config/server.hpp"
#include "error.hpp"

class ServiceEndpointTest : public testing::Test {};

using ClientEndpointsBuilder = launchdarkly::client::EndpointsBuilder;

using ServerEndpointsBuilder = launchdarkly::server::EndpointsBuilder;

using launchdarkly::Error;

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
    ServerEndpointsBuilder builder;
    auto eps = builder.build();
    ASSERT_TRUE(eps);
    ASSERT_EQ(eps->polling_base_url(), "https://sdk.launchdarkly.com");
    ASSERT_EQ(eps->streaming_base_url(), "https://stream.launchdarkly.com");
    ASSERT_EQ(eps->events_base_url(), "https://events.launchdarkly.com");
}

TEST(ServiceEndpointTest, ModifySingleURLCausesError) {
    auto result = ClientEndpointsBuilder().polling_base_url("foo").build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_AllURLsMustBeSet);

    result = ClientEndpointsBuilder().streaming_base_url("foo").build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_AllURLsMustBeSet);

    result = ClientEndpointsBuilder().events_base_url("foo").build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_AllURLsMustBeSet);
}

TEST(ServiceEndpointsTest, RelaySetsAllURLS) {
    auto eps = ClientEndpointsBuilder().relay_proxy("foo").build();
    ASSERT_TRUE(eps);
    ASSERT_EQ(eps->streaming_base_url(), "foo");
    ASSERT_EQ(eps->polling_base_url(), "foo");
    ASSERT_EQ(eps->events_base_url(), "foo");
}

TEST(ServiceEndpointsTest, TrimsTrailingSlashes) {
    {
        auto eps = ClientEndpointsBuilder().relay_proxy("foo/").build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->streaming_base_url(), "foo");
    }

    {
        auto eps = ClientEndpointsBuilder().relay_proxy("foo////////").build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->streaming_base_url(), "foo");
    }

    {
        auto eps = ClientEndpointsBuilder().relay_proxy("/").build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->streaming_base_url(), "");
    }
}

TEST(ServiceEndpointsTest, EmptyURLsAreInvalid) {
    auto result = ClientEndpointsBuilder().relay_proxy("").build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_EmptyURL);

    result = ClientEndpointsBuilder()
                 .streaming_base_url("")
                 .events_base_url("foo")
                 .polling_base_url("bar")
                 .build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_EmptyURL);

    result = ClientEndpointsBuilder()
                 .streaming_base_url("foo")
                 .events_base_url("")
                 .polling_base_url("bar")
                 .build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_EmptyURL);

    result = ClientEndpointsBuilder()
                 .streaming_base_url("foo")
                 .events_base_url("bar")
                 .polling_base_url("")
                 .build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_EmptyURL);
}
