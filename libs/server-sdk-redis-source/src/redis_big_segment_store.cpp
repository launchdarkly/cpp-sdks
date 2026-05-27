#include <launchdarkly/server_side/integrations/redis/redis_big_segment_store.hpp>

#include <sw/redis++/redis++.h>

#include <cerrno>
#include <cstdlib>
#include <iterator>
#include <utility>
#include <vector>

namespace launchdarkly::server_side::integrations {

namespace {

// Schema strings from the LaunchDarkly Big Segments spec; must match what
// Relay writes byte-for-byte.
//
// Membership keys are three-part: {user-prefix}:{namespace}:{context-hash},
// where each membership state has its own namespace. Sync-time is two-part:
// {user-prefix}:big_segments_synchronized_on.
constexpr char kIncludeKeyNamespace[] = "big_segment_include";
constexpr char kExcludeKeyNamespace[] = "big_segment_exclude";
constexpr char kSyncTimeKey[] = "big_segments_synchronized_on";

}  // namespace

tl::expected<std::unique_ptr<RedisBigSegmentStore>, std::string>
RedisBigSegmentStore::Create(std::string uri, std::string prefix) {
    try {
        return std::unique_ptr<RedisBigSegmentStore>(new RedisBigSegmentStore(
            std::make_unique<sw::redis::Redis>(std::move(uri)),
            std::move(prefix)));
    } catch (sw::redis::Error const& e) {
        return tl::make_unexpected(e.what());
    }
}

RedisBigSegmentStore::RedisBigSegmentStore(
    std::unique_ptr<sw::redis::Redis> redis,
    std::string prefix)
    : redis_(std::move(redis)),
      prefix_(std::move(prefix)),
      sync_time_key_(prefix_ + ":" + kSyncTimeKey) {}

RedisBigSegmentStore::~RedisBigSegmentStore() = default;

IBigSegmentStore::GetMembershipResult RedisBigSegmentStore::GetMembership(
    std::string const& context_hash) const {
    std::string const include_key =
        prefix_ + ":" + kIncludeKeyNamespace + ":" + context_hash;
    std::string const exclude_key =
        prefix_ + ":" + kExcludeKeyNamespace + ":" + context_hash;

    std::vector<std::string> included;
    std::vector<std::string> excluded;

    try {
        redis_->smembers(include_key, std::back_inserter(included));
        redis_->smembers(exclude_key, std::back_inserter(excluded));
    } catch (sw::redis::Error const& e) {
        return tl::make_unexpected(e.what());
    }

    return Membership::FromSegmentRefs(included, excluded);
}

IBigSegmentStore::GetMetadataResult RedisBigSegmentStore::GetMetadata() const {
    sw::redis::OptionalString raw;
    try {
        raw = redis_->get(sync_time_key_);
    } catch (sw::redis::Error const& e) {
        return tl::make_unexpected(e.what());
    }

    if (!raw) {
        return std::nullopt;
    }

    errno = 0;
    char* end = nullptr;
    long long const parsed = std::strtoll(raw->c_str(), &end, 10);
    if (errno != 0 || end == raw->c_str() || *end != '\0') {
        return tl::make_unexpected(
            "Redis Big Segments synchronized_on is not a valid integer");
    }

    return StoreMetadata{std::chrono::milliseconds{parsed}};
}

}  // namespace launchdarkly::server_side::integrations
