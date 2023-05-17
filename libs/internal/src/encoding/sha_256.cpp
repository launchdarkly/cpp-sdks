#include "openssl/sha.h"

#include <launchdarkly/encoding/sha_256.hpp>

#include <functional>
#include <iomanip>
#include <sstream>

namespace launchdarkly::encoding {

std::string Sha256String(std::string const& input) {
    std::stringstream output_stream;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.c_str(), input.size());
    SHA256_Final(hash, &sha256);
    output_stream << std::hex << std::noshowbase;
    for (unsigned char byte : hash) {
        output_stream << std::setw(2) << std::setfill('0')
                      << static_cast<int>(byte);
    }
    return output_stream.str();
}

}  // namespace launchdarkly::encoding
