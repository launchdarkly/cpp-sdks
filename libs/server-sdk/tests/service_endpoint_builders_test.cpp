#include <gtest/gtest.h>

#include <launchdarkly/error.hpp>
#include <launchdarkly/server_side/config/builders/all_builders.hpp>

using namespace launchdarkly::server_side::config::builders;
using Error = launchdarkly::Error;

TEST(ServiceEndpointTest, DefaultServerBuilderURLs) {
    EndpointsBuilder builder;
    auto eps = builder.Build();
    ASSERT_TRUE(eps);
    ASSERT_EQ(eps->PollingBaseUrl(), "https://sdk.launchdarkly.com");
    ASSERT_EQ(eps->StreamingBaseUrl(), "https://stream.launchdarkly.com");
    ASSERT_EQ(eps->EventsBaseUrl(), "https://events.launchdarkly.com");
}

TEST(ServiceEndpointTest, ModifySingleURLCausesError) {
    auto result = EndpointsBuilder().PollingBaseUrl("foo").Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_AllURLsMustBeSet);

    result = EndpointsBuilder().StreamingBaseUrl("foo").Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_AllURLsMustBeSet);

    result = EndpointsBuilder().EventsBaseUrl("foo").Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_AllURLsMustBeSet);
}

TEST(ServiceEndpointsTest, RelaySetsAllURLS) {
    auto eps = EndpointsBuilder().RelayProxyBaseURL("foo").Build();
    ASSERT_TRUE(eps);
    ASSERT_EQ(eps->StreamingBaseUrl(), "foo");
    ASSERT_EQ(eps->PollingBaseUrl(), "foo");
    ASSERT_EQ(eps->EventsBaseUrl(), "foo");
}

TEST(ServiceEndpointsTest, TrimsTrailingSlashes) {
    {
        auto eps = EndpointsBuilder().RelayProxyBaseURL("foo/").Build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->StreamingBaseUrl(), "foo");
    }

    {
        auto eps = EndpointsBuilder().RelayProxyBaseURL("foo////////").Build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->StreamingBaseUrl(), "foo");
    }

    {
        auto eps = EndpointsBuilder().RelayProxyBaseURL("/").Build();
        ASSERT_TRUE(eps);
        ASSERT_EQ(eps->StreamingBaseUrl(), "");
    }
}

TEST(ServiceEndpointsTest, EmptyURLsAreInvalid) {
    auto result = EndpointsBuilder().RelayProxyBaseURL("").Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_EmptyURL);

    result = EndpointsBuilder()
                 .StreamingBaseUrl("")
                 .EventsBaseUrl("foo")
                 .PollingBaseUrl("bar")
                 .Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_EmptyURL);

    result = EndpointsBuilder()
                 .StreamingBaseUrl("foo")
                 .EventsBaseUrl("")
                 .PollingBaseUrl("bar")
                 .Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_EmptyURL);

    result = EndpointsBuilder()
                 .StreamingBaseUrl("foo")
                 .EventsBaseUrl("bar")
                 .PollingBaseUrl("")
                 .Build();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kConfig_Endpoints_EmptyURL);
}
