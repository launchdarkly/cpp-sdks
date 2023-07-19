#include <openssl/sha.h>

#include <launchdarkly/encoding/sha_256.hpp>

namespace launchdarkly::encoding {

std::array<unsigned char, SHA256_DIGEST_LENGTH> Sha256String(
    std::string const& input) {
    std::array<unsigned char, SHA256_DIGEST_LENGTH> hash{};

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.c_str(), input.size());
    SHA256_Final(hash.data(), &sha256);

    return hash;
}

}  // namespace launchdarkly::encoding
