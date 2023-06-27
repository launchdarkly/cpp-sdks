#include "evaluation_stack.hpp"

namespace launchdarkly::evaluation::detail {

EvaluationStack::EvaluationStack(std::size_t initial_bucket_count)
    : prerequisites_seen_(initial_bucket_count),
      segments_seen_(initial_bucket_count) {}

void EvaluationStack::NoticePrerequisite(std::string const& prerequisite_key) {
    prerequisites_seen_.insert(prerequisite_key);
}
void EvaluationStack::NoticeSegment(std::string const& segment_key) {
    segments_seen_.insert(segment_key);
}

bool EvaluationStack::SeenPrerequisite(
    std::string const& prerequisite_key) const {
    return prerequisites_seen_.find(prerequisite_key) !=
           prerequisites_seen_.end();
}

bool EvaluationStack::SeenSegment(std::string const& segment_key) const {
    return segments_seen_.find(segment_key) != segments_seen_.end();
}

}  // namespace launchdarkly::evaluation::detail
