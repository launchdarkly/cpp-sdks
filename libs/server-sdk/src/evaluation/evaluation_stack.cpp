#include "evaluation_stack.hpp"

namespace launchdarkly::server_side::evaluation {

Guard::Guard(std::unordered_set<std::string>& set, std::string key)
    : set_(set), key_(std::move(key)) {
    set_.insert(key_);
}

Guard::~Guard() {
    set_.erase(key_);
}

std::optional<Guard> EvaluationStack::NoticePrerequisite(
    std::string prerequisite_key) {
    if (prerequisites_seen_.count(prerequisite_key) != 0) {
        return std::nullopt;
    }
    return std::make_optional<Guard>(prerequisites_seen_,
                                     std::move(prerequisite_key));
}

std::optional<Guard> EvaluationStack::NoticeSegment(std::string segment_key) {
    if (segments_seen_.count(segment_key) != 0) {
        return std::nullopt;
    }
    return std::make_optional<Guard>(segments_seen_, std::move(segment_key));
}

}  // namespace launchdarkly::server_side::evaluation
