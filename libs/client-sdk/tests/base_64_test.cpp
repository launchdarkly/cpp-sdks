#include "base_64.hpp"
#include <gtest/gtest.h>

using launchdarkly::client_side::Base64UrlEncode;

TEST(Base64Encoding, CanEncodeString) {
    // Test vectors from RFC4668
    // https://datatracker.ietf.org/doc/html/rfc4648#section-10
    EXPECT_EQ("", Base64UrlEncode(""));
    EXPECT_EQ("Zg==", Base64UrlEncode("f"));
    EXPECT_EQ("Zm8=", Base64UrlEncode("fo"));
    EXPECT_EQ("Zm9v", Base64UrlEncode("foo"));
    EXPECT_EQ("Zm9vYg==", Base64UrlEncode("foob"));
    EXPECT_EQ("Zm9vYmE=", Base64UrlEncode("fooba"));
    EXPECT_EQ("Zm9vYmFy", Base64UrlEncode("foobar"));
}
