#pragma once

#include <launchdarkly/server_side/integrations/dynamodb/options.hpp>

#include <aws/dynamodb/DynamoDBClient.h>

#include <tl/expected.hpp>

#include <memory>
#include <string>

namespace launchdarkly::server_side::integrations::detail {

// Builds an Aws::DynamoDB::DynamoDBClient configured from the supplied
// DynamoDBClientOptions, or returns an error string if the options are
// internally inconsistent (e.g. only one of aws_access_key_id /
// aws_secret_access_key is set).
//
// Caller is responsible for ensuring AwsSdkGuard::Ensure() has been called
// first.
tl::expected<std::shared_ptr<Aws::DynamoDB::DynamoDBClient>, std::string>
BuildDynamoDBClient(DynamoDBClientOptions const& options);

}  // namespace launchdarkly::server_side::integrations::detail
