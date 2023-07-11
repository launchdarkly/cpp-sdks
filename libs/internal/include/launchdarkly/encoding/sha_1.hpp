#pragma once
#include <array>
#include <string>

#include <openssl/sha.h>

namespace launchdarkly::encoding {
std::array<unsigned char, SHA_DIGEST_LENGTH> Sha1String(
    std::string const& input);
}
