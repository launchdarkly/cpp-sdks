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
inline constexpr char kItemAttribute[] = "item";

// Sentinel namespace marking the table as initialized. When a prefix is set,
// both the partition-key value AND the sort-key value are prefixed:
//   {namespace: "myprefix:$inited", key: "myprefix:$inited"}.
inline constexpr char kInitedNamespace[] = "$inited";

// Big Segments schema. Membership rows use partition key
// "{prefix}:big_segments_user" and sort key {context_hash}, with
// "included" / "excluded" String Set attributes naming segment refs. The
// metadata row uses partition key AND sort key both set to
// "{prefix}:big_segments_metadata", with the sync timestamp stored as a
// Number under "synchronizedOn".
inline constexpr char kBigSegmentsUserNamespace[] = "big_segments_user";
inline constexpr char kBigSegmentsMetadataNamespace[] = "big_segments_metadata";
inline constexpr char kBigSegmentsIncludedAttribute[] = "included";
inline constexpr char kBigSegmentsExcludedAttribute[] = "excluded";
inline constexpr char kBigSegmentsSyncTimeAttribute[] = "synchronizedOn";

}  // namespace launchdarkly::server_side::integrations::detail
