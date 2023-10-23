#include <openssl/sha.h>

#include <launchdarkly/encoding/sha_256.hpp>

namespace launchdarkly::encoding {

std::array<unsigned char, SHA_DIGEST_LENGTH> Sha1String(
    std::string const& input) {
    std::array<unsigned char, SHA_DIGEST_LENGTH> hash{};

    SHA_CTX sha;
    SHA1_Init(&sha);
    SHA1_Update(&sha, input.c_str(), input.size());
    SHA1_Final(hash.data(), &sha);

    return hash;
}
}  // namespace launchdarkly::encoding
