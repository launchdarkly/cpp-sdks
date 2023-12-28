#include <openssl/sha.h>

#include <launchdarkly/encoding/sha_256.hpp>

namespace launchdarkly::encoding {

std::array<unsigned char, SHA256_DIGEST_LENGTH> Sha256String(
    std::string const& input) {
    std::array<unsigned char, SHA256_DIGEST_LENGTH> hash{};

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    SHA256(reinterpret_cast<unsigned char const*>(input.data()), input.size(),
           hash.data());

    return hash;
}

}  // namespace launchdarkly::encoding
