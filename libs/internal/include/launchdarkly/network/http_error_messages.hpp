#pragma once

#include <sstream>

#include <launchdarkly/context.hpp>
#include "http_requester.hpp"

namespace launchdarkly::network {

static bool IsInvalidSdkKeyStatus(HttpResult::StatusCode code);

/**
 * Get an error message for an HTTP error.
 * @param code The status code of the error.
 * @param context The context of the error, for example a "polling request".
 * @param retry_message The retry message, or nullopt if it will not be retried.
 * @return The error message.
 */
std::string ErrorForStatusCode(HttpResult::StatusCode code,
                               std::string context,
                               std::optional<std::string> retry_message);

}  // namespace launchdarkly::network
