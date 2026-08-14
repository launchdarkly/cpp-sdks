/** @file options.hpp
 * @brief Options for constructing a DynamoDB-backed integration.
 */

#pragma once

#include <optional>
#include <string>

namespace launchdarkly::server_side::integrations {

/**
 * @brief Optional knobs for constructing the AWS DynamoDB client used by
 * @ref DynamoDBDataSource (and other DynamoDB-backed integrations).
 *
 * When unset, fields fall through to the AWS SDK's defaults:
 *
 * - @ref region resolves via the SDK region provider chain (environment,
 *   shared config file, instance metadata).
 * - @ref endpoint defaults to the standard AWS DynamoDB endpoint for the
 *   resolved region. Set it to point at DynamoDB Local or LocalStack, e.g.
 *   `http://localhost:8000`.
 * - If none of @ref aws_access_key_id / @ref aws_secret_access_key /
 *   @ref aws_session_token are set, the SDK's default credential provider
 *   chain is used (environment variables, shared credentials file, EC2/ECS
 *   roles).
 */
struct DynamoDBClientOptions {
    std::optional<std::string> region;
    std::optional<std::string> endpoint;
    std::optional<std::string> aws_access_key_id;
    std::optional<std::string> aws_secret_access_key;
    std::optional<std::string> aws_session_token;
};

}  // namespace launchdarkly::server_side::integrations
