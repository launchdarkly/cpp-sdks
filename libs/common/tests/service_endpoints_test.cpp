#include <gtest/gtest.h>

#include <launchdarkly/config/client.hpp>
#include <launchdarkly/error.hpp>

class ServiceEndpointTest : public testing::Test {};

using namespace launchdarkly;
using launchdarkly::Error;

TEST(ServiceEndpointTest, DefaultClientBuilderURLs) {
    client_side::EndpointsBuilder builder;
    auto eps = builder.Build();
    ASSERT_TRUE(eps);
    ASSERT_EQ(eps->PollingBaseUrl(), "https://clientsdk.launchdarkly.com");
    ASSERT_EQ(eps->StreamingBaseUrl(), "https://clientstream.launchdarkly.com");
    ASSERT_EQ(eps->EventsBaseUrl(), "https://mobile.launchdarkly.com");
}

TEST(ServiceEndpointTest, ModifySingleURLCausesError) {
    auto result = client_side::EndpointsBuilder().PollingBaseUrl("foo").Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), ErrorCode::kConfig_Endpoints_AllURLsMustBeSet);

    result = client_side::EndpointsBuilder().StreamingBaseUrl("foo").Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), ErrorCode::kConfig_Endpoints_AllURLsMustBeSet);

    result = client_side::EndpointsBuilder().EventsBaseUrl("foo").Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), ErrorCode::kConfig_Endpoints_AllURLsMustBeSet);
}

TEST(ServiceEndpointsTest, RelaySetsAllURLS) {
    auto eps = client_side::EndpointsBuilder().RelayProxyBaseURL("foo").Build();
    ASSERT_TRUE(eps);
    ASSERT_EQ(eps->StreamingBaseUrl(), "foo");
    ASSERT_EQ(eps->PollingBaseUrl(), "foo");
    ASSERT_EQ(eps->EventsBaseUrl(), "foo");
}

TEST(ServiceEndpointsTest, TrimsTrailingSlashes) {
    {
        auto eps =
            client_side::EndpointsBuilder().RelayProxyBaseURL("foo/").Build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->StreamingBaseUrl(), "foo");
    }

    {
        auto eps = client_side::EndpointsBuilder()
                       .RelayProxyBaseURL("foo////////")
                       .Build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->StreamingBaseUrl(), "foo");
    }

    {
        auto eps =
            client_side::EndpointsBuilder().RelayProxyBaseURL("/").Build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->StreamingBaseUrl(), "");
    }
}

TEST(ServiceEndpointsTest, EmptyURLsAreInvalid) {
    auto result = client_side::EndpointsBuilder().RelayProxyBaseURL("").Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), ErrorCode::kConfig_Endpoints_EmptyURL);

    result = client_side::EndpointsBuilder()
                 .StreamingBaseUrl("")
                 .EventsBaseUrl("foo")
                 .PollingBaseUrl("bar")
                 .Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), ErrorCode::kConfig_Endpoints_EmptyURL);

    result = client_side::EndpointsBuilder()
                 .StreamingBaseUrl("foo")
                 .EventsBaseUrl("")
                 .PollingBaseUrl("bar")
                 .Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), ErrorCode::kConfig_Endpoints_EmptyURL);

    result = client_side::EndpointsBuilder()
                 .StreamingBaseUrl("foo")
                 .EventsBaseUrl("bar")
                 .PollingBaseUrl("")
                 .Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), ErrorCode::kConfig_Endpoints_EmptyURL);
}
