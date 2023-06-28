#pragma once

#include <string>
#include <unordered_set>

namespace launchdarkly::server_side::evaluation::detail {

struct Guard {
    Guard(std::unordered_set<std::string>& set, std::string const& key);
    ~Guard();

    Guard(Guard const&) = delete;
    Guard& operator=(Guard const&) = delete;

    Guard(Guard&&) = delete;
    Guard& operator=(Guard&&) = delete;

   private:
    std::unordered_set<std::string>& set_;
    std::string const& key_;
};

class EvaluationStack {
   public:
    EvaluationStack(std::size_t initial_bucket_count);

    [[nodiscard]] Guard NoticePrerequisite(std::string const& prerequisite_key);
    [[nodiscard]] Guard NoticeSegment(std::string const& segment_key);

    bool SeenPrerequisite(std::string const& prerequisite_key) const;
    bool SeenSegment(std::string const& segment_key) const;

   private:
    std::unordered_set<std::string> prerequisites_seen_;
    std::unordered_set<std::string> segments_seen_;
};

}  // namespace launchdarkly::server_side::evaluation::detail
