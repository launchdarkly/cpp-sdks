#pragma once

#include <string>
#include <unordered_set>

namespace launchdarkly::evaluation::detail {

class EvaluationStack {
   public:
    EvaluationStack(std::size_t initial_bucket_count);

    void NoticePrerequisite(std::string const& prerequisite_key);
    void NoticeSegment(std::string const& segment_key);

    bool SeenPrerequisite(std::string const& prerequisite_key) const;
    bool SeenSegment(std::string const& segment_key) const;

   private:
    std::unordered_set<std::string> prerequisites_seen_;
    std::unordered_set<std::string> segments_seen_;
};

}  // namespace launchdarkly::evaluation::detail
