#pragma once

#include <string>

namespace launchdarkly::server_side {

/**
 * Name of the HTTP header used to identify this SDK instance for the purpose of
 * estimating server-connection-minutes when polling. The value is a v4 UUID
 * that is generated once per SDK instance and remains constant for the
 * lifetime of the client.
 *
 * See: sdk-specs / SCMP-server-connection-minutes-polling.
 */
inline constexpr char const* kInstanceIdHeader = "X-LaunchDarkly-Instance-Id";

/**
 * Generate a fresh v4 UUID suitable for use as the value of the
 * X-LaunchDarkly-Instance-Id header. Each call returns a new identifier;
 * callers are expected to generate the value exactly once per SDK instance
 * and reuse it for the lifetime of that instance.
 *
 * @return A string formatted as a lowercase v4 UUID, e.g.
 *         "550e8400-e29b-41d4-a716-446655440000".
 */
[[nodiscard]] std::string MakeInstanceId();

}  // namespace launchdarkly::server_side
