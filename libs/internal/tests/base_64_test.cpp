#include <gtest/gtest.h>

#include "launchdarkly/encoding/base_64.hpp"

using launchdarkly::encoding::Base64Encode;
using launchdarkly::encoding::Base64UrlEncode;

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

TEST(Base64Encoding, StandardCanEncodeString) {
    // Test vectors from RFC4668
    // https://datatracker.ietf.org/doc/html/rfc4648#section-10
    EXPECT_EQ("", Base64Encode(""));
    EXPECT_EQ("Zg==", Base64Encode("f"));
    EXPECT_EQ("Zm8=", Base64Encode("fo"));
    EXPECT_EQ("Zm9v", Base64Encode("foo"));
    EXPECT_EQ("Zm9vYg==", Base64Encode("foob"));
    EXPECT_EQ("Zm9vYmE=", Base64Encode("fooba"));
    EXPECT_EQ("Zm9vYmFy", Base64Encode("foobar"));
}

TEST(Base64Encoding, StandardUsesNonUrlSafeAlphabet) {
    // "???" encodes to a value ending in '/' under the standard alphabet and
    // '_' under the URL-safe one; ">>>" exercises '+' versus '-'.
    EXPECT_EQ("Pz8/", Base64Encode("???"));
    EXPECT_EQ("Pz8_", Base64UrlEncode("???"));
    EXPECT_EQ("Pj4+", Base64Encode(">>>"));
    EXPECT_EQ("Pj4-", Base64UrlEncode(">>>"));
}
