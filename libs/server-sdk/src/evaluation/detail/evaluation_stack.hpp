#pragma once

#include <optional>
#include <string>
#include <unordered_set>

namespace launchdarkly::server_side::evaluation::detail {

/**
 * Guard is an object used to track that a segment or flag key has been noticed.
 * Upon destruction, the key is forgotten.
 */
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

/**
 * EvaluationStack is used to track which segments and flags have been noticed
 * during evaluation in order to detect circular references.
 *
 * Intended usage is to first call Seen[Segment|Prerequisite] to check if the
 * segment/flag was already seen during this evaluation.
 *
 * If so, this indicates a circular reference. If not, call
 * Notice[Segment|Prerequisite] to mark it as seen for the duration of the
 * returned Guard's lifetime.
 */
class EvaluationStack {
   public:
    EvaluationStack() = default;

    /**
     * Marks a prerequisite as noticed. The prerequisite will be forgotten when
     * the returned Guard destructs.
     *
     * Must only be called if Seen returns false.
     *
     * @param prerequisite_key Key of the prerequisite.
     * @return Guard object representing the fact that a prerequisite has been
     * seen for the duration of the returned object's lifetime.
     */
    [[nodiscard]] std::optional<Guard> NoticePrerequisite(
        std::string const& prerequisite_key);

    /**
     * Marks a segment as noticed. The segment will be forgotten when the
     * returned Guard destructs.
     *
     * Must only be called if Seen returns false.
     *
     * @param segment_key Key of the segment.
     * @return Guard object representing the fact that a segment has been seen
     * for the duration of the returned object's lifetime.
     */
    [[nodiscard]] std::optional<Guard> NoticeSegment(
        std::string const& segment_key);

   private:
    std::unordered_set<std::string> prerequisites_seen_;
    std::unordered_set<std::string> segments_seen_;
};

}  // namespace launchdarkly::server_side::evaluation::detail
