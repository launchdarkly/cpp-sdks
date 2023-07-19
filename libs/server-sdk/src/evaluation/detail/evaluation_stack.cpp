#include "evaluation_stack.hpp"

namespace launchdarkly::server_side::evaluation::detail {

Guard::Guard(std::unordered_set<std::string>& set, std::string const& key)
    : set_(set), key_(key) {
    set_.insert(key_);
}

Guard::~Guard() {
    set_.erase(key_);
}

std::optional<Guard> EvaluationStack::NoticePrerequisite(
    std::string const& prerequisite_key) {
    if (prerequisites_seen_.count(prerequisite_key) != 0) {
        return std::nullopt;
    }
    return std::make_optional<Guard>(prerequisites_seen_, prerequisite_key);
}

std::optional<Guard> EvaluationStack::NoticeSegment(
    std::string const& segment_key) {
    if (segments_seen_.count(segment_key) != 0) {
        return std::nullopt;
    }
    return std::make_optional<Guard>(segments_seen_, segment_key);
}

}  // namespace launchdarkly::server_side::evaluation::detail
