#include <gtest/gtest.h>

#include "launchdarkly/encoding/base_16.hpp"
#include "launchdarkly/encoding/sha_1.hpp"

using namespace launchdarkly::encoding;

TEST(Sha1, CanEncodeString) {
    // Test vectors from
    // https://www.di-mgt.com.au/sha_testvectors.html
    EXPECT_EQ(std::string("da39a3ee5e6b4b0d3255bfef95601890afd80709"),
              Base16Encode(Sha1String("")));

    EXPECT_EQ(std::string("a9993e364706816aba3e25717850c26c9cd0d89d"),
              Base16Encode(Sha1String("abc")));

    EXPECT_EQ(std::string("84983e441c3bd26ebaae4aa1f95129e5e54670f1"),
              Base16Encode(Sha1String(
                  "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")));
}
