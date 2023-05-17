#pragma once

#include <openssl/sha.h>
#include <string>

namespace launchdarkly::encoding {

std::string Sha256String(std::string const& input);

}  // namespace launchdarkly::encoding