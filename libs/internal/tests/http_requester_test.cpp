#include <gtest/gtest.h>

#include <launchdarkly/config/shared/builders/http_properties_builder.hpp>
#include <launchdarkly/config/shared/sdks.hpp>
#include <launchdarkly/network/http_requester.hpp>

using launchdarkly::config::shared::ClientSDK;
using launchdarkly::config::shared::builders::HttpPropertiesBuilder;
using launchdarkly::network::AppendQueryParam;
using launchdarkly::network::AppendUrl;
using launchdarkly::network::HttpMethod;
using launchdarkly::network::HttpRequest;

TEST(HttpRequestTests, NormalizesRelativeUrl) {
    HttpRequest normalized(
        "https://some.domain.com/potato/../ham?egg=true&cheese=true",
        launchdarkly::network::HttpMethod::kGet,
        HttpPropertiesBuilder<ClientSDK>().Build(), std::nullopt);

    EXPECT_EQ("some.domain.com", normalized.Host());
    EXPECT_EQ("/ham?egg=true&cheese=true", normalized.Path());
}

TEST(HttpRequestTests, UsesCorrectPort) {
    HttpRequest a("scheme://some.domain.com:123",
                  launchdarkly::network::HttpMethod::kGet,
                  HttpPropertiesBuilder<ClientSDK>().Build(), std::nullopt);

    EXPECT_EQ("123", a.Port());

    HttpRequest b("scheme://some.domain.com:456",
                  launchdarkly::network::HttpMethod::kGet,
                  HttpPropertiesBuilder<ClientSDK>().Build(), std::nullopt);

    EXPECT_EQ("456", b.Port());

    HttpRequest c("scheme://some.domain.com",
                  launchdarkly::network::HttpMethod::kGet,
                  HttpPropertiesBuilder<ClientSDK>().Build(), std::nullopt);

    EXPECT_FALSE(c.Port());
}

TEST(HttpRequestTests, DetectsHttpsFromScheme) {
    HttpRequest secure(
        "https://some.domain.com", launchdarkly::network::HttpMethod::kGet,
        HttpPropertiesBuilder<ClientSDK>().Build(), std::nullopt);

    EXPECT_TRUE(secure.Https());

    HttpRequest insecure(
        "http://some.domain.com", launchdarkly::network::HttpMethod::kGet,
        HttpPropertiesBuilder<ClientSDK>().Build(), std::nullopt);

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

// The Beast backend sends Path() verbatim as the request target, so any
// percent-encoding a URL builder applied must survive. A server-supplied
// value such as the FDv2 "basis" selector state is the realistic input.
TEST(HttpRequestTests, PathPreservesPercentEncodedQuery) {
    HttpRequest request(
        "https://some.domain.com/sdk/poll/eval"
        "?basis=x%0D%0AX-Injected:%201&f=a%26b%20c%23d",
        launchdarkly::network::HttpMethod::kGet,
        HttpPropertiesBuilder<ClientSDK>().Build(), std::nullopt);

    EXPECT_EQ("/sdk/poll/eval?basis=x%0D%0AX-Injected:%201&f=a%26b%20c%23d",
              request.Path());
    EXPECT_EQ(
        "https://some.domain.com/sdk/poll/eval"
        "?basis=x%0D%0AX-Injected:%201&f=a%26b%20c%23d",
        request.Url());
}

// Path normalization may decode escapes of characters that are legal in a
// path (older Boost releases include %2F), so only characters that cannot
// appear raw in a request target are checked here.
TEST(HttpRequestTests, PathPreservesPercentEncodedPathSegments) {
    HttpRequest request(
        "https://some.domain.com/ld%20relay/p%23q/sdk/latest-all",
        launchdarkly::network::HttpMethod::kGet,
        HttpPropertiesBuilder<ClientSDK>().Build(), std::nullopt);

    EXPECT_EQ("/ld%20relay/p%23q/sdk/latest-all", request.Path());
}

TEST(HttpRequestTests, PathOmitsAnEmptyQuery) {
    HttpRequest request("https://some.domain.com/potato?",
                        launchdarkly::network::HttpMethod::kGet,
                        HttpPropertiesBuilder<ClientSDK>().Build(),
                        std::nullopt);

    EXPECT_EQ("/potato", request.Path());
}

TEST(HttpRequestTests, AppendKeepsTheEncodingOfTheUrl) {
    EXPECT_EQ("https://the.url.com/ld%20relay/sdk/latest-all?tok=a%26b",
              AppendUrl("https://the.url.com/ld%20relay?tok=a%26b",
                        "/sdk/latest-all"));

    EXPECT_EQ("https://the.url.com/100%25/x/y",
              AppendUrl("https://the.url.com/100%25/x", "y"));
}

// The appended path is not percent-encoded, so every character that is not
// allowed in a path segment is encoded, and a '%' is data.
TEST(HttpRequestTests, AppendEncodesTheAppendedPath) {
    EXPECT_EQ("https://the.url.com/has%20space",
              AppendUrl("https://the.url.com", "/has space"));

    EXPECT_EQ("https://the.url.com/base/p%2523q",
              AppendUrl("https://the.url.com/base", "p%23q"));

    EXPECT_EQ("https://the.url.com/a%0D%0Ab",
              AppendUrl("https://the.url.com", "a\r\nb"));
}

// The client SDK appends a base64url encoded context, which can end in '='.
TEST(HttpRequestTests, AppendKeepsBase64UrlPadding) {
    EXPECT_EQ("https://the.url.com/msdk/evalx/contexts/eyJrZXkiOiJhIn0=",
              AppendUrl("https://the.url.com/msdk/evalx/contexts",
                        "eyJrZXkiOiJhIn0="));
}

TEST(HttpRequestTests, AppendQueryParamUsesTheRightSeparator) {
    EXPECT_EQ("https://the.url.com/x?withReasons=true",
              AppendQueryParam("https://the.url.com/x", "withReasons", "true"));

    // A base URL that already carries a query keeps it.
    EXPECT_EQ("https://the.url.com/x?tok=a%26b&filter=my-filter",
              AppendQueryParam("https://the.url.com/x?tok=a%26b", "filter",
                               "my-filter"));
}

// Every character outside the unreserved set is encoded, so the result does
// not depend on which characters a URL library release leaves raw.
TEST(HttpRequestTests, AppendQueryParamEncodesReservedCharacters) {
    EXPECT_EQ(
        "https://the.url.com/x?basis=a%26b%20c%23d%0D%0A",
        AppendQueryParam("https://the.url.com/x", "basis", "a&b c#d\r\n"));

    EXPECT_EQ("https://the.url.com/x?basis=a%20b%2Bc%3Bd%3De%2Ff%3Fg%25h~i",
              AppendQueryParam("https://the.url.com/x", "basis",
                               "a b+c;d=e/f?g%h~i"));
}

TEST(HttpRequestTests, AppendQueryParamPropagatesInvalidUrls) {
    EXPECT_EQ(std::nullopt, AppendQueryParam(std::nullopt, "a", "b"));
    EXPECT_EQ(std::nullopt, AppendQueryParam("not a url", "a", "b"));
}
