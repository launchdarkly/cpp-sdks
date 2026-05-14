#pragma once

#include <launchdarkly/server_side/integrations/dynamodb/options.hpp>

#include <aws/dynamodb/DynamoDBClient.h>

#include <memory>

namespace launchdarkly::server_side::integrations::detail {

// Builds an Aws::DynamoDB::DynamoDBClient configured from the supplied
// DynamoDBClientOptions. Caller is responsible for ensuring AwsSdkGuard::Ensure()
// has been called first.
//
// The shared_ptr return enables future sharing of a single client across
// multiple DynamoDB-backed stores in the same process (e.g. data source +
// big-segment store); today each consumer constructs its own.
std::shared_ptr<Aws::DynamoDB::DynamoDBClient> BuildDynamoDBClient(
    DynamoDBClientOptions const& options);

}  // namespace launchdarkly::server_side::integrations::detail
