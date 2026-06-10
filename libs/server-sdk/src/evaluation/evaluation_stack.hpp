#pragma once

#include <launchdarkly/data/evaluation_reason.hpp>
#include <launchdarkly/server_side/integrations/big_segments/big_segment_store_types.hpp>

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace launchdarkly::server_side::data_components {
class BigSegmentStoreWrapper;
}  // namespace launchdarkly::server_side::data_components

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
 * EvaluationStack holds the per-evaluation state for a single top-level flag
 * evaluation: the prerequisite/segment chains used for circular-reference
 * detection, plus the Big Segments status and membership cache that a Big
 * Segment lookup populates.
 *
 * Not thread-safe: a fresh instance is created per top-level evaluation and is
 * never shared across threads.
 */
class EvaluationStack {
   public:
    /**
     * @param big_segment_store Non-owning pointer to the Big Segment store
     * wrapper, or nullptr if no store is configured. Must outlive the stack.
     */
    explicit EvaluationStack(
        data_components::BigSegmentStoreWrapper* big_segment_store = nullptr);

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

    /**
     * @return The Big Segment store wrapper, or nullptr if none is configured.
     */
    [[nodiscard]] data_components::BigSegmentStoreWrapper* BigSegmentStore()
        const;

    /**
     * Records the status of a Big Segment lookup. If multiple lookups occur in
     * one evaluation, the least-trustworthy status wins (NOT_CONFIGURED >
     * STORE_ERROR > STALE > HEALTHY).
     */
    void RecordBigSegmentsStatus(enum EvaluationReason::BigSegmentsStatus status);

    /**
     * @return The aggregated Big Segments status, or std::nullopt if no Big
     * Segment was queried during this evaluation.
     */
    [[nodiscard]] std::optional<enum EvaluationReason::BigSegmentsStatus> BigSegmentsStatus() const;

    /**
     * Returns the cached membership for a context key looked up earlier in this
     * evaluation, or nullptr if that key has not been queried yet.
     */
    [[nodiscard]] integrations::Membership const* FindMembership(
        std::string const& context_key) const;

    /**
     * Caches a context key's membership so later Big Segment lookups for the
     * same key in this evaluation reuse it instead of re-querying the store.
     */
    void StoreMembership(std::string context_key,
                         integrations::Membership membership);

    /**
     * Records that the Big Segment store returned an error for the given
     * context key during this evaluation. Subsequent Big Segment lookups for
     * the same key must be treated as non-matches without re-querying.
     */
    void RecordStoreError(std::string context_key);

    /**
     * @return True if a Big Segment store lookup for the given context key has
     * already errored during this evaluation.
     */
    [[nodiscard]] bool DidStoreError(std::string const& context_key) const;

   private:
    std::unordered_set<std::string> prerequisites_seen_;
    std::unordered_set<std::string> segments_seen_;

    data_components::BigSegmentStoreWrapper* big_segment_store_;
    std::optional<enum EvaluationReason::BigSegmentsStatus> big_segments_status_;
    // Keyed by unhashed context key. Empty until the first Big Segment lookup.
    std::unordered_map<std::string, integrations::Membership> memberships_;
    std::unordered_set<std::string> store_error_keys_;
};

}  // namespace launchdarkly::server_side::evaluation
