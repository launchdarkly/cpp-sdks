#include <launchdarkly/server_side/integrations/dynamodb/dynamodb_source.hpp>

#include <aws/dynamodb/DynamoDBClient.h>

namespace launchdarkly::server_side::integrations {

// Touch an Aws::DynamoDB type so the linker actually pulls in the AWS SDK and
// we prove the dependency wires up. This function is intentionally never
// called; it exists solely to validate the CMake/CI scaffolding.
void DynamoDBSourceLinkSmoke() {
    Aws::DynamoDB::DynamoDBClient* unused = nullptr;
    (void)unused;
}

}  // namespace launchdarkly::server_side::integrations
