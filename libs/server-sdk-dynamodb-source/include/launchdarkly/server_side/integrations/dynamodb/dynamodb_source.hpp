#pragma once

namespace launchdarkly::server_side::integrations {

// Scaffold-only entry point. The real DynamoDBDataSource class will replace
// this in a subsequent PR; this declaration exists so the smoke .cpp has
// something to define and the library produces a non-empty archive.
void DynamoDBSourceLinkSmoke();

}  // namespace launchdarkly::server_side::integrations
