#include "rules.hpp"
#include "bucketing.hpp"
#include "operators.hpp"

namespace launchdarkly::server_side::evaluation {

using namespace data_model;

bool MaybeNegate(Clause const& clause, bool value) {
    if (clause.negate) {
        return !value;
    }
    return value;
}

tl::expected<bool, Error> Match(Flag::Rule const& rule,
                                launchdarkly::Context const& context,
                                data_store::IDataStore const* store,
                                detail::EvaluationStack& stack) {
    for (Clause const& clause : rule.clauses) {
        tl::expected<bool, Error> result = Match(clause, context, store, stack);
        if (!result) {
            return result;
        }
        if (!(*result)) {
            return false;
        }
    }
    return true;
}

tl::expected<bool, Error> Match(Segment::Rule const& rule,
                                Context const& context,
                                data_store::IDataStore const* store,
                                detail::EvaluationStack& stack,
                                std::string const& key,
                                std::string const& salt) {
    for (Clause const& clause : rule.clauses) {
        auto maybe_match = Match(clause, context, store, stack);
        if (!maybe_match) {
            return tl::make_unexpected(maybe_match.error());
        }
        if (!(*maybe_match)) {
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
        return bucket < (*rule.weight / 100000.0);
    }

    return true;
}

tl::expected<bool, Error> Match(Clause const& clause,
                                launchdarkly::Context const& context,
                                data_store::IDataStore const* store,
                                detail::EvaluationStack& stack) {
    if (clause.op == Clause::Op::kSegmentMatch) {
        return MatchSegment(clause, context, store, stack);
    }
    return MatchNonSegment(clause, context);
}

tl::expected<bool, Error> MatchSegment(Clause const& clause,
                                       launchdarkly::Context const& context,
                                       data_store::IDataStore const* store,
                                       detail::EvaluationStack& stack) {
    for (Value const& value : clause.values) {
        // A segment key represented as a Value is a string; non-strings are
        // ignored.
        if (value.Type() != Value::Type::kString) {
            continue;
        }

        std::string const& segment_key = value.AsString();

        std::shared_ptr<data_store::SegmentDescriptor> segment_ptr =
            store->GetSegment(segment_key);

        if (!segment_ptr || !segment_ptr->item) {
            // Segments that don't exist are ignored.
            continue;
        }

        auto maybe_contains =
            Contains(*segment_ptr->item, context, store, stack);

        if (!maybe_contains) {
            return tl::make_unexpected(maybe_contains.error());
        }

        if (*maybe_contains) {
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
                                   data_store::IDataStore const* store,
                                   detail::EvaluationStack& stack) {
    auto guard = stack.NoticeSegment(segment.key);
    if (!guard) {
        return tl::make_unexpected(Error::CyclicSegmentReference(segment.key));
    }

    if (segment.unbounded) {
        // TODO: set big segment status to NOT_CONFIGURED.
        return false;
    }

    if (IsTargeted(context, segment.included, segment.includedContexts)) {
        return true;
    }

    if (IsTargeted(context, segment.excluded, segment.excludedContexts)) {
        return false;
    }

    for (auto const& rule : segment.rules) {
        // TODO(cwaldren): return error if salt is missing
        if (Match(rule, context, store, stack, segment.key, *segment.salt)) {
            return true;
        }
    }

    return false;
}

bool IsTargeted(Context const& context,
                std::vector<std::string> const& keys,
                std::vector<Segment::Target> const& targets) {
    if (IsUser(context) && targets.empty()) {
        return std::find(keys.begin(), keys.end(), context.CanonicalKey()) !=
               keys.end();
    }

    for (auto const& target : targets) {
        Value const& key = context.Get(target.contextKind, "key");
        if (!key.IsString()) {
            continue;
        }
        if (std::find(target.values.begin(), target.values.end(), key) !=
            target.values.end()) {
            return true;
        }
    }

    return false;
}

bool IsUser(Context const& context) {
    auto const& kinds = context.Kinds();
    return kinds.size() == 1 && kinds[0] == "user";
}

}  // namespace launchdarkly::server_side::evaluation
