#include "rules.hpp"
#include "bucketing.hpp"
#include "operators.hpp"

#include "../data_components/big_segments/big_segment_store_wrapper.hpp"

#include <optional>
#include <string>
#include <utility>

namespace launchdarkly::server_side::evaluation {

using namespace data_model;

namespace {

// Maps the wrapper's internal status to the public reason status.
enum EvaluationReason::BigSegmentsStatus ToBigSegmentsStatus(
    data_components::BigSegmentsStatus status) {
    switch (status) {
        case data_components::BigSegmentsStatus::kHealthy:
            return EvaluationReason::BigSegmentsStatus::kHealthy;
        case data_components::BigSegmentsStatus::kStale:
            return EvaluationReason::BigSegmentsStatus::kStale;
        case data_components::BigSegmentsStatus::kStoreError:
            return EvaluationReason::BigSegmentsStatus::kStoreError;
        case data_components::BigSegmentsStatus::kNotConfigured:
            return EvaluationReason::BigSegmentsStatus::kNotConfigured;
    }
    return EvaluationReason::BigSegmentsStatus::kHealthy;
}

std::string MakeBigSegmentRef(Segment const& segment) {
    return segment.key + ".g" + std::to_string(*segment.generation);
}

// Evaluates membership in an unbounded (Big) segment. Returns true/false for a
// definite match/non-match, or std::nullopt when the membership has no entry
// for this segment and evaluation should fall through to the segment's rules.
std::optional<bool> MatchBigSegment(Segment const& segment,
                                    Context const& context,
                                    EvaluationStack& stack) {
    if (!segment.generation) {
        // Without a generation the segment ref can't be formed.
        stack.RecordBigSegmentsStatus(
            EvaluationReason::BigSegmentsStatus::kNotConfigured);
        return false;
    }

    // An absent or empty unboundedContextKind defaults to "user".
    ContextKind const kind = (segment.unboundedContextKind &&
                              !segment.unboundedContextKind->t.empty())
                                 ? *segment.unboundedContextKind
                                 : ContextKind{"user"};
    Value const& context_key = context.Get(kind, "key");
    if (!context_key.IsString()) {
        return false;
    }
    std::string const& key = context_key.AsString();

    if (stack.DidStoreError(key)) {
        return false;
    }

    integrations::Membership const* membership = stack.FindMembership(key);
    if (!membership) {
        auto* store = stack.BigSegmentStore();
        if (!store) {
            stack.RecordBigSegmentsStatus(
                EvaluationReason::BigSegmentsStatus::kNotConfigured);
            return false;
        }
        auto result = store->GetMembership(key);
        auto const status = ToBigSegmentsStatus(result.status);
        stack.RecordBigSegmentsStatus(status);
        if (status == EvaluationReason::BigSegmentsStatus::kStoreError) {
            stack.RecordStoreError(key);
            return false;
        }
        stack.StoreMembership(key, std::move(result.membership));
        membership = stack.FindMembership(key);
    }

    return membership->CheckMembership(MakeBigSegmentRef(segment));
}

}  // namespace

bool MaybeNegate(Clause const& clause, bool value) {
    if (clause.negate) {
        return !value;
    }
    return value;
}

tl::expected<bool, Error> Match(Flag::Rule const& rule,
                                launchdarkly::Context const& context,
                                data_interfaces::IStore const& store,
                                EvaluationStack& stack) {
    for (Clause const& clause : rule.clauses) {
        tl::expected<bool, Error> result = Match(clause, context, store, stack);
        if (!result) {
            return result;
        }
        if (!(result.value())) {
            return false;
        }
    }
    return true;
}

tl::expected<bool, Error> Match(Segment::Rule const& rule,
                                Context const& context,
                                data_interfaces::IStore const& store,
                                EvaluationStack& stack,
                                std::string const& key,
                                std::string const& salt) {
    for (Clause const& clause : rule.clauses) {
        auto maybe_match = Match(clause, context, store, stack);
        if (!maybe_match) {
            return tl::make_unexpected(maybe_match.error());
        }
        if (!(maybe_match.value())) {
            return false;
        }
    }

    if (rule.weight && rule.weight >= 0.0) {
        BucketPrefix prefix(key, salt);
        auto maybe_bucket = Bucket(context, rule.bucketBy, prefix, false,
                                   rule.rolloutContextKind);
        if (!maybe_bucket) {
            return tl::make_unexpected(maybe_bucket.error());
        }
        auto [bucket, ignored] = *maybe_bucket;
        return bucket < (*rule.weight / kBucketScale);
    }

    return true;
}

tl::expected<bool, Error> Match(Clause const& clause,
                                launchdarkly::Context const& context,
                                data_interfaces::IStore const& store,
                                EvaluationStack& stack) {
    if (clause.op == Clause::Op::kSegmentMatch) {
        return MatchSegment(clause, context, store, stack);
    }
    return MatchNonSegment(clause, context);
}

tl::expected<bool, Error> MatchSegment(Clause const& clause,
                                       launchdarkly::Context const& context,
                                       data_interfaces::IStore const& store,
                                       EvaluationStack& stack) {
    for (Value const& value : clause.values) {
        // A segment key represented as a Value is a string; non-strings are
        // ignored.
        if (value.Type() != Value::Type::kString) {
            continue;
        }

        std::string const& segment_key = value.AsString();

        std::shared_ptr<data_model::SegmentDescriptor> segment_ptr =
            store.GetSegment(segment_key);

        if (!segment_ptr || !segment_ptr->item) {
            // Segments that don't exist are ignored.
            continue;
        }

        auto maybe_contains =
            Contains(*segment_ptr->item, context, store, stack);

        if (!maybe_contains) {
            return tl::make_unexpected(maybe_contains.error());
        }

        if (maybe_contains.value()) {
            return MaybeNegate(clause, true);
        }
    }

    return MaybeNegate(clause, false);
}

tl::expected<bool, Error> MatchNonSegment(
    Clause const& clause,
    launchdarkly::Context const& context) {
    if (!clause.attribute.Valid()) {
        return tl::make_unexpected(
            Error::InvalidAttributeReference(clause.attribute.RedactionName()));
    }

    if (clause.attribute.IsKind()) {
        for (auto const& clause_value : clause.values) {
            for (auto const& kind : context.Kinds()) {
                if (operators::Match(clause.op, kind, clause_value)) {
                    return MaybeNegate(clause, true);
                }
            }
        }
        return MaybeNegate(clause, false);
    }

    Value const& attribute = context.Get(clause.contextKind, clause.attribute);
    if (attribute.IsNull()) {
        return false;
    }

    if (attribute.IsArray()) {
        for (Value const& clause_value : clause.values) {
            for (Value const& context_value : attribute.AsArray()) {
                if (operators::Match(clause.op, context_value, clause_value)) {
                    return MaybeNegate(clause, true);
                }
            }
        }
        return MaybeNegate(clause, false);
    }

    if (std::any_of(clause.values.begin(), clause.values.end(),
                    [&](Value const& clause_value) {
                        return operators::Match(clause.op, attribute,
                                                clause_value);
                    })) {
        return MaybeNegate(clause, true);
    }

    return MaybeNegate(clause, false);
}

tl::expected<bool, Error> Contains(Segment const& segment,
                                   Context const& context,
                                   data_interfaces::IStore const& store,
                                   EvaluationStack& stack) {
    auto guard = stack.NoticeSegment(segment.key);
    if (!guard) {
        return tl::make_unexpected(Error::CyclicSegmentReference(segment.key));
    }

    if (segment.unbounded) {
        if (auto match = MatchBigSegment(segment, context, stack)) {
            return *match;
        }
        // Big segments don't use the regular include/exclude target lists; a
        // membership miss falls through directly to the segment's rules.
    } else {
        if (IsTargeted(context, segment.included, segment.includedContexts)) {
            return true;
        }

        if (IsTargeted(context, segment.excluded, segment.excludedContexts)) {
            return false;
        }
    }

    for (auto const& rule : segment.rules) {
        if (!segment.salt) {
            return tl::make_unexpected(Error::MissingSalt(segment.key));
        }
        tl::expected<bool, Error> maybe_match =
            Match(rule, context, store, stack, segment.key, *segment.salt);
        if (!maybe_match) {
            return tl::make_unexpected(maybe_match.error());
        }
        if (maybe_match.value()) {
            return true;
        }
    }

    return false;
}

bool IsTargeted(Context const& context,
                std::vector<std::string> const& user_keys,
                std::vector<Segment::Target> const& context_targets) {
    for (auto const& target : context_targets) {
        Value const& key = context.Get(target.contextKind, "key");
        if (!key.IsString()) {
            continue;
        }
        if (std::find(target.values.begin(), target.values.end(), key) !=
            target.values.end()) {
            return true;
        }
    }

    if (auto key = context.Get("user", "key"); !key.IsNull()) {
        return std::find(user_keys.begin(), user_keys.end(), key.AsString()) !=
               user_keys.end();
    }

    return false;
}
}  // namespace launchdarkly::server_side::evaluation
