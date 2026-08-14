#include <gtest/gtest.h>

#include "evaluation/evaluator.hpp"
#include "test_store.hpp"

#include <data_components/big_segments/big_segment_store_wrapper.hpp>
#include <data_components/memory_store/memory_store.hpp>

#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/logging/null_logger.hpp>

#include <boost/asio/io_context.hpp>

#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <string>

using namespace launchdarkly;
using namespace launchdarkly::server_side;
namespace integrations = launchdarkly::server_side::integrations;
namespace built = launchdarkly::server_side::config::built;
using data_components::BigSegmentStoreWrapper;
using namespace std::chrono_literals;

namespace {

// In-memory store for evaluator tests. GetMembership returns queued responses
// in order (the last one sticks once the queue is exhausted); metadata is
// fixed. The store ignores the hashed key it is passed.
class FakeBigSegmentStore : public integrations::IBigSegmentStore {
   public:
    GetMembershipResult GetMembership(
        std::string const&) const noexcept override {
        std::lock_guard lock(mutex_);
        ++membership_calls_;
        if (responses_.size() > 1) {
            auto front = responses_.front();
            responses_.pop_front();
            return front;
        }
        if (!responses_.empty()) {
            return responses_.front();
        }
        return integrations::Membership::FromSegmentRefs({}, {});
    }

    GetMetadataResult GetMetadata() const noexcept override {
        std::lock_guard lock(mutex_);
        return metadata_;
    }

    void PushMembership(GetMembershipResult response) {
        std::lock_guard lock(mutex_);
        responses_.push_back(std::move(response));
    }

    void SetMetadata(GetMetadataResult metadata) {
        std::lock_guard lock(mutex_);
        metadata_ = std::move(metadata);
    }

    int MembershipCalls() const {
        std::lock_guard lock(mutex_);
        return membership_calls_;
    }

   private:
    mutable std::mutex mutex_;
    mutable int membership_calls_ = 0;
    mutable std::deque<GetMembershipResult> responses_;
    // Defaults to fresh metadata so a successful lookup reports HEALTHY.
    GetMetadataResult metadata_ = std::optional<integrations::StoreMetadata>{
        integrations::StoreMetadata{std::chrono::system_clock::now()}};
};

// A membership that includes a context in the given segment ref.
integrations::Membership Included(std::string const& ref) {
    return integrations::Membership::FromSegmentRefs({ref}, {});
}

// A membership that excludes a context from the given segment ref.
integrations::Membership Excluded(std::string const& ref) {
    return integrations::Membership::FromSegmentRefs({}, {ref});
}

std::string UnboundedSegment(std::string const& key,
                             std::string const& kind,
                             bool with_generation,
                             std::string const& rules) {
    std::string json = R"({"key":")" + key +
                       R"(","unbounded":true,"unboundedContextKind":")" + kind +
                       R"(","included":[],"excluded":[],"rules":[)" + rules +
                       R"(],"salt":"salty","version":1)";
    if (with_generation) {
        json += R"(,"generation":1)";
    }
    json += "}";
    return json;
}

// A flag that serves variation 0 (false) when any of its segmentMatch rules
// matches, otherwise its fallthrough variation 1 (true). One rule per segment.
std::string FlagMatchingSegments(std::vector<std::string> const& segment_keys) {
    std::string rules;
    for (std::size_t i = 0; i < segment_keys.size(); ++i) {
        if (i != 0) {
            rules += ",";
        }
        rules += R"({"id":"rule-)" + std::to_string(i) +
                 R"(","clauses":[{"op":"segmentMatch","values":[")" +
                 segment_keys[i] + R"("]}],"variation":0,"trackEvents":false})";
    }
    return R"({"key":"flag","version":42,"on":true,"targets":[],"rules":[)" +
           rules +
           R"(],"prerequisites":[],"fallthrough":{"variation":1},)"
           R"("offVariation":0,"variations":[false,true],"salt":"salty"})";
}

// A segment rule that matches a user whose key is "alice".
char const* kRuleMatchesAlice =
    R"({"id":"seg-rule","clauses":[{"attribute":"key","op":"in",)"
    R"("values":["alice"],"contextKind":"user"}]})";

class BigSegmentEvaluatorTest : public ::testing::Test {
   public:
    BigSegmentEvaluatorTest()
        : logger_(logging::NullLogger()),
          fake_(std::make_shared<FakeBigSegmentStore>()) {
        store_.Init({});
    }

    void UpsertFlag(std::string const& json) {
        store_.Upsert("flag", test_store::Flag(json.c_str()));
    }

    void UpsertSegment(std::string const& key, std::string const& json) {
        store_.Upsert(key, test_store::Segment(json.c_str()));
    }

    // Builds a wrapper over the fake store and an evaluator that uses it.
    evaluation::Evaluator EvaluatorWithStore() {
        built::BigSegmentsConfig config;
        config.store = fake_;
        config.context_cache_size = 1000;
        config.context_cache_time = 5s;
        config.status_poll_interval = 5s;
        config.stale_after = 2min;
        wrapper_ = std::make_shared<BigSegmentStoreWrapper>(
            config, ioc_.get_executor(), logger_);
        return evaluation::Evaluator{logger_, store_, wrapper_.get()};
    }

    // An evaluator with no Big Segment store configured.
    evaluation::Evaluator EvaluatorWithoutStore() {
        return evaluation::Evaluator{logger_, store_};
    }

   protected:
    Logger logger_;
    boost::asio::io_context ioc_;
    data_components::MemoryStore store_;
    std::shared_ptr<FakeBigSegmentStore> fake_;
    std::shared_ptr<BigSegmentStoreWrapper> wrapper_;
};

Context AliceUser() {
    return ContextBuilder().Kind("user", "alice").Build();
}

TEST_F(BigSegmentEvaluatorTest, NoStoreConfiguredIsNotConfigured) {
    UpsertSegment("bigseg", UnboundedSegment("bigseg", "user", true, ""));
    UpsertFlag(FlagMatchingSegments({"bigseg"}));

    auto eval = EvaluatorWithoutStore();
    auto detail =
        eval.Evaluate(store_.GetFlag("flag")->item.value(), AliceUser());

    EXPECT_EQ(*detail, Value(true));  // No match -> fallthrough.
    EXPECT_EQ(detail.Reason()->BigSegmentsStatus(),
              EvaluationReason::BigSegmentsStatus::kNotConfigured);
}

TEST_F(BigSegmentEvaluatorTest, MissingGenerationIsNotConfigured) {
    UpsertSegment("bigseg", UnboundedSegment("bigseg", "user",
                                             /*with_generation=*/false, ""));
    UpsertFlag(FlagMatchingSegments({"bigseg"}));

    auto eval = EvaluatorWithStore();
    auto detail =
        eval.Evaluate(store_.GetFlag("flag")->item.value(), AliceUser());

    EXPECT_EQ(*detail, Value(true));
    EXPECT_EQ(detail.Reason()->BigSegmentsStatus(),
              EvaluationReason::BigSegmentsStatus::kNotConfigured);
    EXPECT_EQ(fake_->MembershipCalls(), 0);  // Never queried the store.
}

TEST_F(BigSegmentEvaluatorTest, ContextLacksUnboundedKindDoesNotQuery) {
    UpsertSegment("bigseg", UnboundedSegment("bigseg", "org", true, ""));
    UpsertFlag(FlagMatchingSegments({"bigseg"}));

    auto eval = EvaluatorWithStore();
    // Context has no "org" kind, so there is no key to look up.
    auto detail =
        eval.Evaluate(store_.GetFlag("flag")->item.value(), AliceUser());

    EXPECT_EQ(*detail, Value(true));
    EXPECT_EQ(detail.Reason()->BigSegmentsStatus(),
              EvaluationReason::BigSegmentsStatus::kNone);
    EXPECT_EQ(fake_->MembershipCalls(), 0);
}

TEST_F(BigSegmentEvaluatorTest, IncludedMembershipMatches) {
    UpsertSegment("bigseg", UnboundedSegment("bigseg", "user", true, ""));
    UpsertFlag(FlagMatchingSegments({"bigseg"}));
    fake_->PushMembership(Included("bigseg.g1"));

    auto eval = EvaluatorWithStore();
    auto detail =
        eval.Evaluate(store_.GetFlag("flag")->item.value(), AliceUser());

    EXPECT_EQ(*detail, Value(false));  // Segment match -> rule variation.
    EXPECT_EQ(detail.Reason()->Kind(), EvaluationReason::Kind::kRuleMatch);
    EXPECT_EQ(detail.Reason()->BigSegmentsStatus(),
              EvaluationReason::BigSegmentsStatus::kHealthy);
}

TEST_F(BigSegmentEvaluatorTest, ExcludedMembershipDoesNotMatch) {
    UpsertSegment("bigseg", UnboundedSegment("bigseg", "user", true, ""));
    UpsertFlag(FlagMatchingSegments({"bigseg"}));
    fake_->PushMembership(Excluded("bigseg.g1"));

    auto eval = EvaluatorWithStore();
    auto detail =
        eval.Evaluate(store_.GetFlag("flag")->item.value(), AliceUser());

    EXPECT_EQ(*detail, Value(true));  // Excluded -> fallthrough.
    EXPECT_EQ(detail.Reason()->BigSegmentsStatus(),
              EvaluationReason::BigSegmentsStatus::kHealthy);
}

TEST_F(BigSegmentEvaluatorTest, NoMembershipEntryFallsThroughToRules) {
    UpsertSegment("bigseg",
                  UnboundedSegment("bigseg", "user", true, kRuleMatchesAlice));
    UpsertFlag(FlagMatchingSegments({"bigseg"}));
    // Membership has no entry for bigseg.g1, so the segment's rules decide.
    fake_->PushMembership(integrations::Membership::FromSegmentRefs({}, {}));

    auto eval = EvaluatorWithStore();
    auto detail =
        eval.Evaluate(store_.GetFlag("flag")->item.value(), AliceUser());

    EXPECT_EQ(*detail, Value(false));  // Segment rule matches alice.
    EXPECT_EQ(detail.Reason()->Kind(), EvaluationReason::Kind::kRuleMatch);
    EXPECT_EQ(detail.Reason()->BigSegmentsStatus(),
              EvaluationReason::BigSegmentsStatus::kHealthy);
}

TEST_F(BigSegmentEvaluatorTest, RegularIncludeListIgnoredForBigSegment) {
    // A big segment's regular include list must be ignored; only store
    // membership (and then the segment's rules) decide matching.
    std::string const segment =
        R"({"key":"bigseg","unbounded":true,"unboundedContextKind":"user",)"
        R"("included":["alice"],"excluded":[],"rules":[],"salt":"salty",)"
        R"("version":1,"generation":1})";
    UpsertSegment("bigseg", segment);
    UpsertFlag(FlagMatchingSegments({"bigseg"}));
    // The store has no entry for alice, so the regular include list must not
    // produce a match.
    fake_->PushMembership(integrations::Membership::FromSegmentRefs({}, {}));

    auto eval = EvaluatorWithStore();
    auto detail =
        eval.Evaluate(store_.GetFlag("flag")->item.value(), AliceUser());

    EXPECT_EQ(*detail, Value(true));  // Include list ignored -> fallthrough.
    EXPECT_EQ(detail.Reason()->BigSegmentsStatus(),
              EvaluationReason::BigSegmentsStatus::kHealthy);
}

TEST_F(BigSegmentEvaluatorTest, StaleStoreReportsStale) {
    UpsertSegment("bigseg", UnboundedSegment("bigseg", "user", true, ""));
    UpsertFlag(FlagMatchingSegments({"bigseg"}));
    fake_->PushMembership(Included("bigseg.g1"));
    // Last update older than stale_after (2min) -> stale.
    fake_->SetMetadata(
        integrations::StoreMetadata{std::chrono::system_clock::now() - 1h});

    auto eval = EvaluatorWithStore();
    auto detail =
        eval.Evaluate(store_.GetFlag("flag")->item.value(), AliceUser());

    EXPECT_EQ(*detail, Value(false));  // Still matches; only trust differs.
    EXPECT_EQ(detail.Reason()->BigSegmentsStatus(),
              EvaluationReason::BigSegmentsStatus::kStale);
}

TEST_F(BigSegmentEvaluatorTest, StoreErrorReportsStoreError) {
    UpsertSegment("bigseg", UnboundedSegment("bigseg", "user", true, ""));
    UpsertFlag(FlagMatchingSegments({"bigseg"}));
    fake_->PushMembership(tl::make_unexpected(std::string("boom")));

    auto eval = EvaluatorWithStore();
    auto detail =
        eval.Evaluate(store_.GetFlag("flag")->item.value(), AliceUser());

    EXPECT_EQ(*detail, Value(true));  // Empty membership -> fallthrough.
    EXPECT_EQ(detail.Reason()->BigSegmentsStatus(),
              EvaluationReason::BigSegmentsStatus::kStoreError);
}

TEST_F(BigSegmentEvaluatorTest, StatusResolvesByWorstPrecedence) {
    UpsertSegment("bigsegA", UnboundedSegment("bigsegA", "user", true, ""));
    UpsertSegment("bigsegB", UnboundedSegment("bigsegB", "org", true, ""));
    UpsertFlag(FlagMatchingSegments({"bigsegA", "bigsegB"}));
    // First lookup (user key) succeeds with no entry -> HEALTHY; second lookup
    // (org key) errors -> STORE_ERROR. STORE_ERROR must win.
    fake_->PushMembership(integrations::Membership::FromSegmentRefs({}, {}));
    fake_->PushMembership(tl::make_unexpected(std::string("boom")));

    auto context =
        ContextBuilder().Kind("user", "alice").Kind("org", "org1").Build();
    auto eval = EvaluatorWithStore();
    auto detail = eval.Evaluate(store_.GetFlag("flag")->item.value(), context);

    EXPECT_EQ(*detail, Value(true));  // Neither segment matched.
    EXPECT_EQ(detail.Reason()->BigSegmentsStatus(),
              EvaluationReason::BigSegmentsStatus::kStoreError);
    EXPECT_EQ(fake_->MembershipCalls(), 2);
}

TEST_F(BigSegmentEvaluatorTest, QueriesStoreOncePerContextKey) {
    // Two big segments of the same kind resolve to the same context key.
    UpsertSegment("bigsegA", UnboundedSegment("bigsegA", "user", true, ""));
    UpsertSegment("bigsegB", UnboundedSegment("bigsegB", "user", true, ""));
    UpsertFlag(FlagMatchingSegments({"bigsegA", "bigsegB"}));
    fake_->PushMembership(integrations::Membership::FromSegmentRefs({}, {}));

    auto eval = EvaluatorWithStore();
    auto detail =
        eval.Evaluate(store_.GetFlag("flag")->item.value(), AliceUser());

    EXPECT_EQ(*detail, Value(true));
    EXPECT_EQ(fake_->MembershipCalls(), 1);
}

}  // namespace
