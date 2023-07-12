#include "evaluation_stack.hpp"

namespace launchdarkly::server_side::evaluation::detail {

Guard::Guard(std::unordered_set<std::string>& set, std::string const& key)
    : set_(set), key_(key) {
    set_.insert(key_);
}

Guard::~Guard() {
    set_.erase(key_);
}

Guard EvaluationStack::NoticePrerequisite(std::string const& prerequisite_key) {
    return {prerequisites_seen_, prerequisite_key};
}

Guard EvaluationStack::NoticeSegment(std::string const& segment_key) {
    return {segments_seen_, segment_key};
}

bool EvaluationStack::SeenPrerequisite(
    std::string const& prerequisite_key) const {
    return prerequisites_seen_.count(prerequisite_key) != 0;
}

bool EvaluationStack::SeenSegment(std::string const& segment_key) const {
    return segments_seen_.count(segment_key) != 0;
}

}  // namespace launchdarkly::server_side::evaluation::detail
