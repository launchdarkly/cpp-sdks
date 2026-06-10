#include "evaluation_stack.hpp"

namespace launchdarkly::server_side::evaluation {

namespace {
// Ranks statuses by how little they can be trusted, so the least-trustworthy
// status wins when several Big Segments are queried in one evaluation
// (NOT_CONFIGURED > STORE_ERROR > STALE > HEALTHY). Note this ordering is
// independent of the enum's underlying values.
int Precedence(enum EvaluationReason::BigSegmentsStatus status) {
    switch (status) {
        case EvaluationReason::BigSegmentsStatus::kHealthy:
            return 0;
        case EvaluationReason::BigSegmentsStatus::kStale:
            return 1;
        case EvaluationReason::BigSegmentsStatus::kStoreError:
            return 2;
        case EvaluationReason::BigSegmentsStatus::kNotConfigured:
            return 3;
    }
    return 0;
}
}  // namespace

EvaluationStack::EvaluationStack(
    data_components::BigSegmentStoreWrapper* big_segment_store)
    : big_segment_store_(big_segment_store) {}

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

data_components::BigSegmentStoreWrapper* EvaluationStack::BigSegmentStore()
    const {
    return big_segment_store_;
}

void EvaluationStack::RecordBigSegmentsStatus(enum EvaluationReason::BigSegmentsStatus status) {
    if (!big_segments_status_ ||
        Precedence(status) > Precedence(*big_segments_status_)) {
        big_segments_status_ = status;
    }
}

std::optional<enum EvaluationReason::BigSegmentsStatus>
EvaluationStack::BigSegmentsStatus() const {
    return big_segments_status_;
}

integrations::Membership const* EvaluationStack::FindMembership(
    std::string const& context_key) const {
    auto const it = memberships_.find(context_key);
    if (it == memberships_.end()) {
        return nullptr;
    }
    return &it->second;
}

void EvaluationStack::StoreMembership(std::string context_key,
                                      integrations::Membership membership) {
    memberships_.emplace(std::move(context_key), std::move(membership));
}

void EvaluationStack::RecordStoreError(std::string context_key) {
    store_error_keys_.insert(std::move(context_key));
}

bool EvaluationStack::DidStoreError(std::string const& context_key) const {
    return store_error_keys_.find(context_key) != store_error_keys_.end();
}

}  // namespace launchdarkly::server_side::evaluation
