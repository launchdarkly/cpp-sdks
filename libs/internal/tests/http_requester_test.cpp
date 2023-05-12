#include <gtest/gtest.h>

#include <launchdarkly/config/shared/builders/http_properties_builder.hpp>
#include <launchdarkly/network/detail/http_requester.hpp>
#include "launchdarkly/config/shared/sdks.hpp"

using launchdarkly::config::detail::ClientSDK;
using launchdarkly::config::detail::builders::HttpPropertiesBuilder;
using launchdarkly::network::detail::AppendUrl;
using launchdarkly::network::detail::HttpMethod;
using launchdarkly::network::detail::HttpRequest;

TEST(HttpRequestTests, NormalizesRelativeUrl) {
    HttpRequest normalized(
        "https://some.domain.com/potato/../ham?egg=true&cheese=true",
        launchdarkly::network::detail::HttpMethod::kGet,
        HttpPropertiesBuilder<ClientSDK>().Build(), std::nullopt);

    EXPECT_EQ("some.domain.com", normalized.Host());
    EXPECT_EQ("/ham?egg=true&cheese=true", normalized.Path());
}

TEST(HttpRequestTests, UsesCorrectPort) {
    HttpRequest a("scheme://some.domain.com:123",
                  launchdarkly::network::detail::HttpMethod::kGet,
                  HttpPropertiesBuilder<ClientSDK>().Build(), std::nullopt);

    EXPECT_EQ("123", a.Port());

    HttpRequest b("scheme://some.domain.com:456",
                  launchdarkly::network::detail::HttpMethod::kGet,
                  HttpPropertiesBuilder<ClientSDK>().Build(), std::nullopt);

    EXPECT_EQ("456", b.Port());

    HttpRequest c("scheme://some.domain.com",
                  launchdarkly::network::detail::HttpMethod::kGet,
                  HttpPropertiesBuilder<ClientSDK>().Build(), std::nullopt);

    EXPECT_FALSE(c.Port());
}

TEST(HttpRequestTests, DetectsHttpsFromScheme) {
    HttpRequest secure("https://some.domain.com",
                       launchdarkly::network::detail::HttpMethod::kGet,
                       HttpPropertiesBuilder<ClientSDK>().Build(),
                       std::nullopt);

    EXPECT_TRUE(secure.Https());

    HttpRequest insecure("http://some.domain.com",
                         launchdarkly::network::detail::HttpMethod::kGet,
                         HttpPropertiesBuilder<ClientSDK>().Build(),
                         std::nullopt);

    EXPECT_FALSE(insecure.Https());
}

TEST(HttpRequestTests, CanAppendBasicPath) {
    EXPECT_EQ("https://the.url.com/potato",
              AppendUrl("https://the.url.com", "/potato"));

    EXPECT_EQ("https://the.url.com/potato",
              AppendUrl("https://the.url.com/", "potato"));

    EXPECT_EQ("https://the.url.com/potato",
              AppendUrl("https://the.url.com/", "/potato"));

    EXPECT_EQ("https://the.url.com/ham/potato",
              AppendUrl("https://the.url.com/ham", "/potato"));
}

TEST(HttpRequestTests, AppendEmpty) {
    EXPECT_EQ("https://the.url.com", AppendUrl("https://the.url.com", ""));
}

TEST(HttpRequestTests, AppendRelativeUrls) {
    EXPECT_EQ("https://the.url.com/cheese",
              AppendUrl("https://the.url.com/ham", "../cheese"));

    EXPECT_EQ("https://the.url.com/cheese",
              AppendUrl("https://the.url.com/ham/", "../cheese"));

    EXPECT_EQ("https://the.url.com/cheese",
              AppendUrl("https://the.url.com/ham", "/../cheese"));

    EXPECT_EQ("https://the.url.com/cheese",
              AppendUrl("https://the.url.com/ham/", "/../cheese"));
}

TEST(HttpRequestTests, CanAppendWithParameters) {
    EXPECT_EQ("https://the.url.com/cheese?ham=true&egg=true",
              AppendUrl("https://the.url.com?ham=true&egg=true", "cheese"));
}
