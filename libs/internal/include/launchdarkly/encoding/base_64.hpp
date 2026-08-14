#pragma once

#include <string>

namespace launchdarkly::encoding {

/**
 * Return a base64 encoded version of the input string.
 * This version is URL safe, which means where a typical '+' or '/' are used
 * instead a '-' or '/' will be used.
 * @param input The string to Base64 encode.
 * @return The encoded value.
 */
std::string Base64UrlEncode(std::string const& input);

/**
 * Return a standard base64 encoded version of the input string, using the
 * RFC 4648 section 4 alphabet (with '+' and '/'). Unlike @ref Base64UrlEncode,
 * this is NOT URL-safe.
 * @param input The string to Base64 encode.
 * @return The encoded value.
 */
std::string Base64Encode(std::string const& input);

}  // namespace launchdarkly::encoding
