#pragma once

namespace launchdarkly::server_side::integrations::detail {

// Schema constants for the LaunchDarkly DynamoDB table layout, shared with
// other DynamoDB-backed integrations in this library.
//
// The schema is defined in sdk-specs/specs/PS-persistent-store/README.md §
// DynamoDB schema and is the same layout that the LaunchDarkly Relay Proxy
// writes; mismatching any of these strings means the SDK and Relay will not
// agree on what's in the table.
inline constexpr char kPartitionKey[] = "namespace";
inline constexpr char kSortKey[] = "key";
inline constexpr char kVersionAttribute[] = "version";
inline constexpr char kItemAttribute[] = "item";

// Sentinel namespace marking the table as initialized. When a prefix is set,
// both the partition-key value AND the sort-key value are prefixed:
//   {namespace: "myprefix:$inited", key: "myprefix:$inited"}.
inline constexpr char kInitedNamespace[] = "$inited";

}  // namespace launchdarkly::server_side::integrations::detail
