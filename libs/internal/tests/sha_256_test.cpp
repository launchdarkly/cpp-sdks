#include <gtest/gtest.h>

#include "launchdarkly/encoding/sha_256.hpp"

using launchdarkly::encoding::Sha256String;

static std::string HexEncode(std::array<unsigned char, 32> arr) {
    std::stringstream output_stream;
    output_stream << std::hex << std::noshowbase;
    for (unsigned char byte : arr) {
        output_stream << std::setw(2) << std::setfill('0')
                      << static_cast<int>(byte);
    }
    return output_stream.str();
}

TEST(Sha256, CanEncodeString) {
    // Test vectors from
    // https://www.di-mgt.com.au/sha_testvectors.html
    EXPECT_EQ(
        std::string(
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"),
        HexEncode(Sha256String("")));

    EXPECT_EQ(
        std::string(
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"),
        HexEncode(Sha256String("abc")));

    EXPECT_EQ(
        std::string(
            "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"),
        HexEncode(Sha256String(
            "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")));
}
