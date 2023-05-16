#include "hash_encode.hpp"
#include "base_64.hpp"

#include <functional>

namespace launchdarkly::encoding {

std::string HashEncode(std::string input) {
    return Base64UrlEncode(std::to_string(std::hash<std::string>{}(input)));
}

}  // namespace launchdarkly::encoding