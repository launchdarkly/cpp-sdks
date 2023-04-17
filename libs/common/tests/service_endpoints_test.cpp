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
    auto eps = builder.Build();
    ASSERT_TRUE(eps);
    ASSERT_EQ(eps->PollingBaseUrl(), "https://clientsdk.launchdarkly.com");
    ASSERT_EQ(eps->StreamingBaseUrl(), "https://clientstream.launchdarkly.com");
    ASSERT_EQ(eps->EventsBaseUrl(), "https://mobile.launchdarkly.com");
}

TEST(ServiceEndpointTest, DefaultServerBuilderURLs) {
    ServerEndpointsBuilder builder;
    auto eps = builder.Build();
    ASSERT_TRUE(eps);
    ASSERT_EQ(eps->PollingBaseUrl(), "https://sdk.launchdarkly.com");
    ASSERT_EQ(eps->StreamingBaseUrl(), "https://stream.launchdarkly.com");
    ASSERT_EQ(eps->EventsBaseUrl(), "https://events.launchdarkly.com");
}

TEST(ServiceEndpointTest, ModifySingleURLCausesError) {
    auto result = ClientEndpointsBuilder().PollingBaseUrl("foo").Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_AllURLsMustBeSet);

    result = ClientEndpointsBuilder().StreamingBaseUrl("foo").Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_AllURLsMustBeSet);

    result = ClientEndpointsBuilder().EventsBaseUrl("foo").Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_AllURLsMustBeSet);
}

TEST(ServiceEndpointsTest, RelaySetsAllURLS) {
    auto eps = ClientEndpointsBuilder().RelayProxy("foo").Build();
    ASSERT_TRUE(eps);
    ASSERT_EQ(eps->StreamingBaseUrl(), "foo");
    ASSERT_EQ(eps->PollingBaseUrl(), "foo");
    ASSERT_EQ(eps->EventsBaseUrl(), "foo");
}

TEST(ServiceEndpointsTest, TrimsTrailingSlashes) {
    {
        auto eps = ClientEndpointsBuilder().RelayProxy("foo/").Build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->StreamingBaseUrl(), "foo");
    }

    {
        auto eps = ClientEndpointsBuilder().RelayProxy("foo////////").Build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->StreamingBaseUrl(), "foo");
    }

    {
        auto eps = ClientEndpointsBuilder().RelayProxy("/").Build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->StreamingBaseUrl(), "");
    }
}

TEST(ServiceEndpointsTest, EmptyURLsAreInvalid) {
    auto result = ClientEndpointsBuilder().RelayProxy("").Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_EmptyURL);

    result = ClientEndpointsBuilder()
                 .StreamingBaseUrl("")
                 .EventsBaseUrl("foo")
                 .PollingBaseUrl("bar")
                 .Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_EmptyURL);

    result = ClientEndpointsBuilder()
                 .StreamingBaseUrl("foo")
                 .EventsBaseUrl("")
                 .PollingBaseUrl("bar")
                 .Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_EmptyURL);

    result = ClientEndpointsBuilder()
                 .StreamingBaseUrl("foo")
                 .EventsBaseUrl("bar")
                 .PollingBaseUrl("")
                 .Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_EmptyURL);
}
