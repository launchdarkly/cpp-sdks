#include "evaluation_stack.hpp"

namespace launchdarkly::server_side::evaluation::detail {

Guard::Guard(std::unordered_set<std::string>& set, std::string const& key)
    : set_(set), key_(key) {
    set_.insert(key_);
}

Guard::~Guard() {
    set_.erase(key_);
}

EvaluationStack::EvaluationStack(std::size_t initial_bucket_count)
    : prerequisites_seen_(initial_bucket_count),
      segments_seen_(initial_bucket_count) {}

Guard EvaluationStack::NoticePrerequisite(std::string const& prerequisite_key) {
    return Guard(prerequisites_seen_, prerequisite_key);
}

Guard EvaluationStack::NoticeSegment(std::string const& segment_key) {
    return Guard(segments_seen_, segment_key);
}

bool EvaluationStack::SeenPrerequisite(
    std::string const& prerequisite_key) const {
    return prerequisites_seen_.find(prerequisite_key) !=
           prerequisites_seen_.end();
}

bool EvaluationStack::SeenSegment(std::string const& segment_key) const {
    return segments_seen_.find(segment_key) != segments_seen_.end();
}

}  // namespace launchdarkly::server_side::evaluation::detail
