#include "openssl/sha.h"

#include <launchdarkly/encoding/sha_256.hpp>

#include <functional>
#include <iomanip>
#include <sstream>

namespace launchdarkly::encoding {

std::array<unsigned char, SHA256_DIGEST_LENGTH> Sha256String(
    std::string const& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.c_str(), input.size());
    SHA256_Final(hash, &sha256);

    std::array<unsigned char, SHA256_DIGEST_LENGTH> out;
    std::copy(std::begin(hash), std::end(hash), out.begin());
    return out;
}

}  // namespace launchdarkly::encoding
