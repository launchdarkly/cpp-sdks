#pragma once

#include <string>
#include <array>

#include "openssl/sha.h"

namespace launchdarkly::encoding {

std::array<unsigned char, SHA256_DIGEST_LENGTH> Sha256String(std::string const& input);

}  // namespace launchdarkly::encoding
