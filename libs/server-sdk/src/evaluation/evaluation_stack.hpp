#pragma once

#include <optional>
#include <string>
#include <unordered_set>

namespace launchdarkly::server_side::evaluation {

/**
 * Guard is an object used to track that a segment or flag key has been noticed.
 * Upon destruction, the key is forgotten.
 */
struct Guard {
    Guard(std::unordered_set<std::string>& set, std::string key);
    ~Guard();

    Guard(Guard const&) = delete;
    Guard& operator=(Guard const&) = delete;

    Guard(Guard&&) = delete;
    Guard& operator=(Guard&&) = delete;

   private:
    std::unordered_set<std::string>& set_;
    std::string const key_;
};

/**
 * EvaluationStack is used to track which segments and flags have been noticed
 * during evaluation in order to detect circular references.
 */
class EvaluationStack {
   public:
    EvaluationStack() = default;

    /**
     * If the given prerequisite key has not been seen, marks it as seen
     * and returns a Guard object. Otherwise, returns std::nullopt.
     *
     * @param prerequisite_key Key of the prerequisite.
     * @return Guard object if not seen before, otherwise std::nullopt.
     */
    [[nodiscard]] std::optional<Guard> NoticePrerequisite(
        std::string prerequisite_key);

    /**
     * If the given segment key has not been seen, marks it as seen
     * and returns a Guard object. Otherwise, returns std::nullopt.
     *
     * @param prerequisite_key Key of the segment.
     * @return Guard object if not seen before, otherwise std::nullopt.
     */
    [[nodiscard]] std::optional<Guard> NoticeSegment(std::string segment_key);

   private:
    std::unordered_set<std::string> prerequisites_seen_;
    std::unordered_set<std::string> segments_seen_;
};

}  // namespace launchdarkly::server_side::evaluation
