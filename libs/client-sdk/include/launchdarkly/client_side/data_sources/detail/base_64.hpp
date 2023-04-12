#pragma once

#include <string>

namespace launchdarkly::client_side::data_sources::detail {

/**
 * Return a base64 encoded version of the input string.
 * This version is URL safe, which means where a typical '+' or '/' are used
 * instead a '-' or '/' will be used.
 * @param input The string to Base64 encode.
 * @return The encoded value.
 */
std::string Base64UrlEncode(std::string const& input);

}  // namespace launchdarkly::client_size
