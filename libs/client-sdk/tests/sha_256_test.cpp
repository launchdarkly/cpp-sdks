#include "encoding/sha_256.hpp"
#include <gtest/gtest.h>

#include "openssl/sha.h"

using launchdarkly::encoding::Sha256String;

TEST(Sha256, CanEncodeString) {
    // Test vectors from
    // https://www.di-mgt.com.au/sha_testvectors.html
    EXPECT_EQ(
        std::string(
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"),
        Sha256String(""));

    EXPECT_EQ(
        std::string(
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"),
        Sha256String("abc"));

    EXPECT_EQ(
        std::string(
            "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"),
        Sha256String(
            "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"));
}
