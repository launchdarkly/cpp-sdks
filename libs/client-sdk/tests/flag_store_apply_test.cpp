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

    // A second full changeset carrying only flagB.
    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagB", Flag(2, Value("b2"))}},
                              Selector{}},
                /* compute_changes= */ false);

    // flagA is dropped and flagB takes the new value.
    EXPECT_FALSE(store.Get("flagA"));
    EXPECT_EQ(Value("b2"), store.Get("flagB")->item->Detail().Value());
}

TEST(FlagStoreApplyTests, PartialChangeSetMergesIntoExistingData) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))}},
                              Selector{}},
                /* compute_changes= */ false);

    // A partial changeset adding flagB.
    store.Apply(FlagChangeSet{ChangeSetType::kPartial,
                              {FlagChange{"flagB", Flag(1, Value("b"))}},
                              Selector{}},
                /* compute_changes= */ false);

    // flagA is kept and flagB is added.
    EXPECT_EQ(Value("a"), store.Get("flagA")->item->Detail().Value());
    EXPECT_EQ(Value("b"), store.Get("flagB")->item->Detail().Value());
}

TEST(FlagStoreApplyTests, DeleteStoresATombstone) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))}},
                              Selector{}},
                /* compute_changes= */ false);

    // A partial changeset deleting flagA.
    store.Apply(
        FlagChangeSet{ChangeSetType::kPartial,
                      {FlagChange{"flagA", ItemDescriptor{Tombstone{5}}}},
                      Selector{}},
        /* compute_changes= */ false);

    // flagA becomes a tombstone that carries the delete version.
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

    // A partial changeset with a lower version than the stored flag.
    store.Apply(FlagChangeSet{ChangeSetType::kPartial,
                              {FlagChange{"flagA", Flag(2, Value("old"))}},
                              Selector{}},
                /* compute_changes= */ false);

    // The lower-version update still applies.
    EXPECT_EQ(Value("old"), store.Get("flagA")->item->Detail().Value());
}

TEST(FlagStoreApplyTests, ChangeSetSelectorBecomesTheCurrentSelector) {
    FlagStore store;

    // A fresh store has no selector.
    EXPECT_FALSE(store.CurrentSelector().value.has_value());

    // Apply a changeset carrying a selector.
    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))}},
                              SelectorAt(3, "state-3")},
                /* compute_changes= */ false);

    // The store adopts that selector.
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

    // A partial changeset that carries no selector.
    store.Apply(FlagChangeSet{ChangeSetType::kPartial,
                              {FlagChange{"flagA", Flag(2, Value("a2"))}},
                              Selector{}},
                /* compute_changes= */ false);

    // The store discards the selector it had.
    EXPECT_FALSE(store.CurrentSelector().value.has_value());
}

// A "none" intent is not a payload. It confirms the data is current, so both
// the flag data and the selector it was verified against still stand.
TEST(FlagStoreApplyTests, NoneChangeSetLeavesDataAndSelectorUnchanged) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))}},
                              SelectorAt(3, "state-3")},
                /* compute_changes= */ false);

    // Apply a "none" changeset.
    store.Apply(FlagChangeSet{ChangeSetType::kNone, {}, Selector{}},
                /* compute_changes= */ false);

    // The flag data is untouched.
    EXPECT_EQ(Value("a"), store.Get("flagA")->item->Detail().Value());
    // The selector still stands.
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

    // Clear the selector.
    store.ClearSelector();

    // The selector is gone but the flag data remains.
    EXPECT_FALSE(store.CurrentSelector().value.has_value());
    EXPECT_EQ(Value("a"), store.Get("flagA")->item->Detail().Value());
}

TEST(FlagStoreApplyTests, ReportsNoChangesWhenComputeChangesIsFalse) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))}},
                              Selector{}},
                /* compute_changes= */ false);

    // Change flagA with change computation disabled.
    auto events =
        store.Apply(FlagChangeSet{ChangeSetType::kPartial,
                                  {FlagChange{"flagA", Flag(2, Value("a2"))}},
                                  Selector{}},
                    /* compute_changes= */ false);

    // No events are produced.
    EXPECT_TRUE(events.empty());
}

TEST(FlagStoreApplyTests, ReportsAChangedValue) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))}},
                              Selector{}},
                /* compute_changes= */ true);

    // Change flagA's value.
    auto events =
        store.Apply(FlagChangeSet{ChangeSetType::kPartial,
                                  {FlagChange{"flagA", Flag(2, Value("a2"))}},
                                  Selector{}},
                    /* compute_changes= */ true);

    // One event reports flagA moving from "a" to "a2".
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

    // Re-apply flagA with the same value at a new version.
    auto events =
        store.Apply(FlagChangeSet{ChangeSetType::kPartial,
                                  {FlagChange{"flagA", Flag(2, Value("a"))}},
                                  Selector{}},
                    /* compute_changes= */ true);

    // No event, because the value did not change.
    EXPECT_TRUE(events.empty());
}

TEST(FlagStoreApplyTests, ReportsANewFlagAgainstANullOldValue) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))}},
                              Selector{}},
                /* compute_changes= */ true);

    // Add a new flag, flagB.
    auto events =
        store.Apply(FlagChangeSet{ChangeSetType::kPartial,
                                  {FlagChange{"flagB", Flag(1, Value("b"))}},
                                  Selector{}},
                    /* compute_changes= */ true);

    // One event reports flagB appearing against a null old value.
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

    // No events, since this is the starting basis.
    EXPECT_TRUE(events.empty());
    // The data is still stored.
    EXPECT_EQ(Value("a"), store.Get("flagA")->item->Detail().Value());
}

TEST(FlagStoreApplyTests, ReportsADeletedFlag) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))}},
                              Selector{}},
                /* compute_changes= */ true);

    // Delete flagA.
    auto events = store.Apply(
        FlagChangeSet{ChangeSetType::kPartial,
                      {FlagChange{"flagA", ItemDescriptor{Tombstone{5}}}},
                      Selector{}},
        /* compute_changes= */ true);

    // One event reports flagA deleted, carrying its old value.
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

    // No event, because there was nothing to delete.
    EXPECT_TRUE(events.empty());
}

TEST(FlagStoreApplyTests, ReportsFlagsAFullChangeSetOmitsAsDeleted) {
    FlagStore store;

    store.Apply(FlagChangeSet{ChangeSetType::kFull,
                              {FlagChange{"flagA", Flag(1, Value("a"))},
                               FlagChange{"flagB", Flag(1, Value("b"))}},
                              Selector{}},
                /* compute_changes= */ true);

    // A full changeset that omits flagB.
    auto events =
        store.Apply(FlagChangeSet{ChangeSetType::kFull,
                                  {FlagChange{"flagA", Flag(2, Value("a"))}},
                                  Selector{}},
                    /* compute_changes= */ true);

    // One event reports the omitted flagB deleted.
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

    // A partial changeset that omits flagB.
    auto events =
        store.Apply(FlagChangeSet{ChangeSetType::kPartial,
                                  {FlagChange{"flagA", Flag(2, Value("a2"))}},
                                  Selector{}},
                    /* compute_changes= */ true);

    // Only flagA is reported as changed.
    ASSERT_EQ(1u, events.size());
    EXPECT_EQ("flagA", events[0].FlagName());
    // flagB is left in place.
    EXPECT_EQ(Value("b"), store.Get("flagB")->item->Detail().Value());
}
