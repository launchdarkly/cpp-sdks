#include <gtest/gtest.h>

#include <launchdarkly/data/evaluation_detail_internal.hpp>
#include <launchdarkly/data/evaluation_result.hpp>

#include "data_sources/data_source_update_sink.hpp"
#include "flag_manager/flag_store.hpp"

using launchdarkly::EvaluationDetailInternal;
using launchdarkly::EvaluationResult;
using launchdarkly::Value;
using launchdarkly::client_side::FlagChange;
using launchdarkly::client_side::FlagChangeSet;
using launchdarkly::client_side::ItemDescriptor;
using launchdarkly::client_side::flag_manager::FlagStore;
using launchdarkly::data_model::ChangeSetType;
using launchdarkly::data_model::Selector;
using Tombstone = launchdarkly::data_model::Tombstone;

static ItemDescriptor Flag(std::uint64_t version, Value value) {
    return ItemDescriptor{
        EvaluationResult{version, std::nullopt, false, false, std::nullopt,
                         EvaluationDetailInternal{std::move(value),
                                                  std::nullopt, std::nullopt}}};
}

static Selector SelectorAt(std::int64_t version, std::string state) {
    return Selector{Selector::State{version, std::move(state)}};
}

TEST(FlagStoreApplyTests, FullChangeSetReplacesAllData) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))},
                               FlagChange{"flagB", Flag(1, Value("b"))}},
                              Selector{}},
                /* compute_changes= */ false);

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagB", Flag(2, Value("b2"))}},
                              Selector{}},
                /* compute_changes= */ false);

    EXPECT_FALSE(store.Get("flagA"));
    EXPECT_EQ(Value("b2"), store.Get("flagB")->item->Detail().Value());
}

TEST(FlagStoreApplyTests, PartialChangeSetMergesIntoExistingData) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))}},
                              Selector{}},
                /* compute_changes= */ false);

    store.Apply(FlagChangeSet{ChangeSetType::kPartial,
                              {FlagChange{"flagB", Flag(1, Value("b"))}},
                              Selector{}},
                /* compute_changes= */ false);

    EXPECT_EQ(Value("a"), store.Get("flagA")->item->Detail().Value());
    EXPECT_EQ(Value("b"), store.Get("flagB")->item->Detail().Value());
}

TEST(FlagStoreApplyTests, NoneChangeSetLeavesDataUnchanged) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))}},
                              SelectorAt(1, "state-1")},
                /* compute_changes= */ false);

    store.Apply(FlagChangeSet{ChangeSetType::kNone, {}, Selector{}},
                /* compute_changes= */ false);

    EXPECT_EQ(Value("a"), store.Get("flagA")->item->Detail().Value());
}

TEST(FlagStoreApplyTests, DeleteStoresATombstone) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))}},
                              Selector{}},
                /* compute_changes= */ false);

    store.Apply(
        FlagChangeSet{ChangeSetType::kPartial,
                      {FlagChange{"flagA", ItemDescriptor{Tombstone{5}}}},
                      Selector{}},
        /* compute_changes= */ false);

    auto descriptor = store.Get("flagA");
    ASSERT_TRUE(descriptor);
    EXPECT_FALSE(descriptor->item.has_value());
    EXPECT_EQ(5u, descriptor->version);
}

// FDv2 reserves the per-flag version for event tracking, so a lower version
// must not cause the apply to reject an update the way FDv1's Upsert does.
TEST(FlagStoreApplyTests, LowerVersionDoesNotRejectTheUpdate) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(10, Value("new"))}},
                              Selector{}},
                /* compute_changes= */ false);

    store.Apply(FlagChangeSet{ChangeSetType::kPartial,
                              {FlagChange{"flagA", Flag(2, Value("old"))}},
                              Selector{}},
                /* compute_changes= */ false);

    EXPECT_EQ(Value("old"), store.Get("flagA")->item->Detail().Value());
}

TEST(FlagStoreApplyTests, SelectorStartsEmpty) {
    FlagStore store;

    EXPECT_FALSE(store.CurrentSelector().value.has_value());
}

TEST(FlagStoreApplyTests, ChangeSetSelectorBecomesTheCurrentSelector) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))}},
                              SelectorAt(3, "state-3")},
                /* compute_changes= */ false);

    auto selector = store.CurrentSelector();
    ASSERT_TRUE(selector.value.has_value());
    EXPECT_EQ(3, selector.value->version);
    EXPECT_EQ("state-3", selector.value->state);
}

TEST(FlagStoreApplyTests, PayloadWithoutASelectorDiscardsTheCurrentSelector) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))}},
                              SelectorAt(3, "state-3")},
                /* compute_changes= */ false);

    store.Apply(FlagChangeSet{ChangeSetType::kPartial,
                              {FlagChange{"flagA", Flag(2, Value("a2"))}},
                              Selector{}},
                /* compute_changes= */ false);

    EXPECT_FALSE(store.CurrentSelector().value.has_value());
}

// A "none" intent is not a payload. It confirms the data is current, so the
// selector it was verified against still stands.
TEST(FlagStoreApplyTests, NoneChangeSetKeepsTheCurrentSelector) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))}},
                              SelectorAt(3, "state-3")},
                /* compute_changes= */ false);

    store.Apply(FlagChangeSet{ChangeSetType::kNone, {}, Selector{}},
                /* compute_changes= */ false);

    auto selector = store.CurrentSelector();
    ASSERT_TRUE(selector.value.has_value());
    EXPECT_EQ("state-3", selector.value->state);
}

TEST(FlagStoreApplyTests, ClearSelectorLeavesFlagDataInPlace) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))}},
                              SelectorAt(3, "state-3")},
                /* compute_changes= */ false);

    store.ClearSelector();

    EXPECT_FALSE(store.CurrentSelector().value.has_value());
    EXPECT_EQ(Value("a"), store.Get("flagA")->item->Detail().Value());
}

TEST(FlagStoreApplyTests, ReportsNoChangesWhenComputeChangesIsFalse) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))}},
                              Selector{}},
                /* compute_changes= */ false);

    auto events =
        store.Apply(FlagChangeSet{ChangeSetType::kPartial,
                                  {FlagChange{"flagA", Flag(2, Value("a2"))}},
                                  Selector{}},
                    /* compute_changes= */ false);

    EXPECT_TRUE(events.empty());
}

TEST(FlagStoreApplyTests, ReportsAChangedValue) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))}},
                              Selector{}},
                /* compute_changes= */ true);

    auto events =
        store.Apply(FlagChangeSet{ChangeSetType::kPartial,
                                  {FlagChange{"flagA", Flag(2, Value("a2"))}},
                                  Selector{}},
                    /* compute_changes= */ true);

    ASSERT_EQ(1u, events.size());
    EXPECT_EQ("flagA", events[0].FlagName());
    EXPECT_EQ(Value("a"), events[0].OldValue());
    EXPECT_EQ(Value("a2"), events[0].NewValue());
    EXPECT_FALSE(events[0].Deleted());
}

TEST(FlagStoreApplyTests, ReportsNothingWhenTheValueIsUnchanged) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))}},
                              Selector{}},
                /* compute_changes= */ true);

    auto events =
        store.Apply(FlagChangeSet{ChangeSetType::kPartial,
                                  {FlagChange{"flagA", Flag(2, Value("a"))}},
                                  Selector{}},
                    /* compute_changes= */ true);

    EXPECT_TRUE(events.empty());
}

TEST(FlagStoreApplyTests, ReportsANewFlagAgainstANullOldValue) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))}},
                              Selector{}},
                /* compute_changes= */ true);

    auto events =
        store.Apply(FlagChangeSet{ChangeSetType::kPartial,
                                  {FlagChange{"flagB", Flag(1, Value("b"))}},
                                  Selector{}},
                    /* compute_changes= */ true);

    ASSERT_EQ(1u, events.size());
    EXPECT_EQ("flagB", events[0].FlagName());
    EXPECT_TRUE(events[0].OldValue().IsNull());
    EXPECT_EQ(Value("b"), events[0].NewValue());
}

// The data the SDK starts from is not a change to what it was evaluating
// before, because it was not evaluating anything.
TEST(FlagStoreApplyTests, ReportsNothingForTheFirstFullChangeSet) {
    FlagStore store;

    auto events =
        store.Apply(FlagChangeSet{ChangeSetType::kFull,
                                  {FlagChange{"flagA", Flag(1, Value("a"))}},
                                  Selector{}},
                    /* compute_changes= */ true);

    EXPECT_TRUE(events.empty());
    EXPECT_EQ(Value("a"), store.Get("flagA")->item->Detail().Value());
}

TEST(FlagStoreApplyTests, ReportsADeletedFlag) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))}},
                              Selector{}},
                /* compute_changes= */ true);

    auto events = store.Apply(
        FlagChangeSet{ChangeSetType::kPartial,
                      {FlagChange{"flagA", ItemDescriptor{Tombstone{5}}}},
                      Selector{}},
        /* compute_changes= */ true);

    ASSERT_EQ(1u, events.size());
    EXPECT_EQ("flagA", events[0].FlagName());
    EXPECT_EQ(Value("a"), events[0].OldValue());
    EXPECT_TRUE(events[0].Deleted());
}

TEST(FlagStoreApplyTests, ReportsNothingWhenDeletingAnAbsentFlag) {
    FlagStore store;

    auto events = store.Apply(
        FlagChangeSet{ChangeSetType::kPartial,
                      {FlagChange{"flagA", ItemDescriptor{Tombstone{5}}}},
                      Selector{}},
        /* compute_changes= */ true);

    EXPECT_TRUE(events.empty());
}

TEST(FlagStoreApplyTests, ReportsFlagsAFullChangeSetOmitsAsDeleted) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))},
                               FlagChange{"flagB", Flag(1, Value("b"))}},
                              Selector{}},
                /* compute_changes= */ true);

    auto events =
        store.Apply(FlagChangeSet{ChangeSetType::kFull,
                                  {FlagChange{"flagA", Flag(2, Value("a"))}},
                                  Selector{}},
                    /* compute_changes= */ true);

    ASSERT_EQ(1u, events.size());
    EXPECT_EQ("flagB", events[0].FlagName());
    EXPECT_EQ(Value("b"), events[0].OldValue());
    EXPECT_TRUE(events[0].Deleted());
}

// A partial changeset only describes the keys it carries, so an absent key is
// untouched rather than deleted.
TEST(FlagStoreApplyTests, ReportsNoDeletionForFlagsAPartialChangeSetOmits) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))},
                               FlagChange{"flagB", Flag(1, Value("b"))}},
                              Selector{}},
                /* compute_changes= */ true);

    auto events =
        store.Apply(FlagChangeSet{ChangeSetType::kPartial,
                                  {FlagChange{"flagA", Flag(2, Value("a2"))}},
                                  Selector{}},
                    /* compute_changes= */ true);

    ASSERT_EQ(1u, events.size());
    EXPECT_EQ("flagA", events[0].FlagName());
    EXPECT_EQ(Value("b"), store.Get("flagB")->item->Detail().Value());
}
