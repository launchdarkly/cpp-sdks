#include "rules.hpp"
#include "bucketing.hpp"
#include "operators.hpp"

namespace launchdarkly::server_side::evaluation {

bool MaybeNegate(data_model::Clause const& clause, bool value) {
    if (clause.negate) {
        return !value;
    }
    return value;
}

bool Match(data_model::Flag::Rule const& rule,
           launchdarkly::Context const& context,
           flag_manager::FlagStore const& store,
           detail::EvaluationStack& stack) {
    return std::all_of(rule.clauses.begin(), rule.clauses.end(),
                       [&](data_model::Clause const& clause) {
                           return Match(clause, context, store, stack);
                       });
}

tl::expected<bool, Error> Match(data_model::Segment::Rule const& rule,
                                Context const& context,
                                flag_manager::FlagStore const& store,
                                detail::EvaluationStack& stack,
                                std::string const& key,
                                std::string const& salt) {
    for (auto const& clause : rule.clauses) {
        auto maybe_match = Match(clause, context, store, stack);
        if (!maybe_match) {
            return tl::make_unexpected(maybe_match.error());
        }
        if (!(*maybe_match)) {
            return false;
        }
    }

    if (rule.weight && rule.weight >= 0.0) {
        auto prefix = BucketPrefix(key, salt);
        auto maybe_bucket = Bucket(context, rule.bucketBy, prefix, false,
                                   rule.rolloutContextKind);
        if (!maybe_bucket) {
            return tl::make_unexpected(maybe_bucket.error());
        }
        auto [bucket, ignored] = *maybe_bucket;
        return bucket < *rule.weight / 100000.0;
    }

    return true;
}

tl::expected<bool, Error> Match(data_model::Clause const& clause,
                                launchdarkly::Context const& context,
                                flag_manager::FlagStore const& store,
                                detail::EvaluationStack& stack) {
    if (clause.op == data_model::Clause::Op::kSegmentMatch) {
        return MatchSegment(clause, context, store, stack);
    } else {
        return MatchNonSegment(clause, context);
    }
}

tl::expected<bool, Error> MatchSegment(data_model::Clause const& clause,
                                       launchdarkly::Context const& context,
                                       flag_manager::FlagStore const& store,
                                       detail::EvaluationStack& stack) {
    for (auto const& value : clause.values) {
        // A segment key represented as a Value is a string.
        if (value.Type() != Value::Type::kString) {
            continue;
        }
        std::string const& segment_key = value.AsString();
        // If there's no such segment, or it existed at one point but was
        // deleted, then move on to the next.
        auto segment_ptr = store.GetSegment(segment_key);
        if (!segment_ptr || !segment_ptr->item) {
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
    data_model::Clause const& clause,
    launchdarkly::Context const& context) {
    if (!clause.attribute.Valid()) {
        return tl::make_unexpected(Error::kInvalidAttributeReference);
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

    auto val = context.Get(clause.contextKind, clause.attribute);
    if (val.Type() == Value::Type::kNull) {
        return false;
    }
    if (val.Type() == Value::Type::kArray) {
        for (auto const& clause_value : clause.values) {
            for (auto const& context_value : val.AsArray()) {
                if (operators::Match(clause.op, context_value, clause_value)) {
                    return MaybeNegate(clause, true);
                }
            }
        }
        return MaybeNegate(clause, false);
    }
    if (std::any_of(clause.values.begin(), clause.values.end(),
                    [&](Value const& clause_value) {
                        return operators::Match(clause.op, val, clause_value);
                    })) {
        return MaybeNegate(clause, true);
    }
    return MaybeNegate(clause, false);
}

tl::expected<bool, Error> Contains(data_model::Segment const& segment,
                                   Context const& context,
                                   flag_manager::FlagStore const& store,
                                   detail::EvaluationStack& stack) {
    if (stack.SeenSegment(segment.key)) {
        return tl::make_unexpected(Error::kCyclicReference);
    }
    auto guard = stack.NoticeSegment(segment.key);

    if (segment.unbounded) {
        return tl::make_unexpected(Error::kBigSegmentEncountered);
    }

    bool is_targeted = false;
    if (IsTargeted(context, segment.included, segment.includedContexts)) {
        is_targeted = true;
    } else if (IsTargeted(context, segment.excluded,
                          segment.excludedContexts)) {
        is_targeted = false;
    } else {
        for (auto const& rule : segment.rules) {
            // TODO: throw error if salt is missing
            if (Match(rule, context, store, stack, segment.key,
                      *segment.salt)) {
                is_targeted = true;
                break;
            }
        }
    }

    return is_targeted;
}

bool IsTargeted(Context const& context,
                std::vector<std::string> const& keys,
                std::vector<data_model::Segment::Target> const& targets) {
    if (IsUser(context) && targets.empty()) {
        return std::any_of(keys.begin(), keys.end(), [&](std::string const& k) {
            return k == context.CanonicalKey();
        });
    }

    for (auto const& target : targets) {
        auto key = context.Get(target.contextKind, "key");
        if (key.Type() != Value::Type::kString) {
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
