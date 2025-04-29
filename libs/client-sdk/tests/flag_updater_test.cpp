#include <gtest/gtest.h>

#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/data/evaluation_detail_internal.hpp>
#include <launchdarkly/data/evaluation_result.hpp>

#include "data_sources/data_source_update_sink.hpp"
#include "flag_manager/flag_store.hpp"
#include "flag_manager/flag_updater.hpp"

using launchdarkly::ContextBuilder;
using launchdarkly::EvaluationDetailInternal;
using launchdarkly::EvaluationResult;
using launchdarkly::Value;
using launchdarkly::client_side::ItemDescriptor;
using launchdarkly::client_side::flag_manager::FlagStore;
using launchdarkly::client_side::flag_manager::FlagUpdater;
using launchdarkly::client_side::flag_manager::FlagValueChangeEvent;
using launchdarkly::client_side::flag_manager::IFlagNotifier;
using Tombstone = launchdarkly::data_model::Tombstone;

TEST(FlagUpdaterDataTests, HandlesEmptyInit) {
    FlagStore manager;
    FlagUpdater updater(manager);

    updater.Init(ContextBuilder().Kind("user", "user-key").Build(),
                 std::unordered_map<std::string, ItemDescriptor>{});

    EXPECT_TRUE(manager.GetAll().empty());
}

TEST(FlagUpdaterDataTests, HandlesInitWithData) {
    FlagStore manager;
    FlagUpdater updater(manager);

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           0, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_EQ("test", manager.Get("flagA")->item.value().Detail().Value());
}

TEST(FlagUpdaterDataTests, HandlesSecondInit) {
    FlagStore manager;
    FlagUpdater updater(manager);

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           0, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagB", ItemDescriptor{EvaluationResult{
                           0, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_EQ("test", manager.Get("flagB")->item.value().Detail().Value());
    EXPECT_FALSE(manager.Get("flagA"));
}

TEST(FlagUpdaterDataTests, HandlePatchNewFlag) {
    FlagStore manager;
    FlagUpdater updater(manager);

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           0, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    updater.Upsert(ContextBuilder().Kind("user", "user-key").Build(), "flagB",
                   ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_EQ("test", manager.Get("flagA")->item.value().Detail().Value());
    EXPECT_EQ("second", manager.Get("flagB")->item.value().Detail().Value());
}

TEST(FlagUpdaterDataTests, HandlePatchUpdateFlag) {
    FlagStore manager;
    FlagUpdater updater(manager);

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           0, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    updater.Upsert(ContextBuilder().Kind("user", "user-key").Build(), "flagA",
                   ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_EQ("second", manager.Get("flagA")->item.value().Detail().Value());
}

TEST(FlagUpdaterDataTests, HandlePatchOutOfOrder) {
    FlagStore manager;
    FlagUpdater updater(manager);

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    updater.Upsert(ContextBuilder().Kind("user", "user-key").Build(), "flagA",
                   ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_EQ("test", manager.Get("flagA")->item.value().Detail().Value());
}

TEST(FlagUpdaterDataTests, HandleDelete) {
    FlagStore manager;
    FlagUpdater updater(manager);

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    updater.Upsert(ContextBuilder().Kind("user", "user-key").Build(), "flagA",
                   ItemDescriptor{Tombstone{2}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_FALSE(manager.Get("flagA")->item.has_value());
}

TEST(FlagUpdaterDataTests, HandleDeleteOutOfOrder) {
    FlagStore manager;
    FlagUpdater updater(manager);

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    updater.Upsert(ContextBuilder().Kind("user", "user-key").Build(), "flagA",
                   ItemDescriptor{Tombstone{0}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_EQ("test", manager.Get("flagA")->item.value().Detail().Value());
}

TEST(FlagUpdaterEventTests, InitialInitProducesNoEvents) {
    FlagStore manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagA", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);
        });

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_FALSE(got_event);
}

TEST(FlagUpdaterEventTests, SecondInitWithUpdateProducesEvents) {
    FlagStore manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagA", [&got_event, &manager](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            EXPECT_EQ("test", event->OldValue().AsString());
            EXPECT_EQ("potato", event->NewValue().AsString());
            EXPECT_EQ("flagA", event->FlagName());
            EXPECT_FALSE(event->Deleted());

            // The value in the store should be consistent with the new value.
            EXPECT_EQ(event->NewValue().AsString(), manager.Get("flagA")
                                                        .get()
                                                        ->item.value()
                                                        .Detail()
                                                        .Value()
                                                        .AsString());
        });

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA",
              ItemDescriptor{EvaluationResult{
                  1, std::nullopt, false, false, std::nullopt,
                  EvaluationDetailInternal{Value("potato"), std::nullopt,
                                           std::nullopt}}}}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_TRUE(got_event);
}

TEST(FlagUpdaterEventTests, SecondInitWithNewFlagProducesEvents) {
    FlagStore manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagB", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            EXPECT_TRUE(event->OldValue().IsNull());
            EXPECT_EQ("potato", event->NewValue().AsString());
            EXPECT_EQ("flagB", event->FlagName());
            EXPECT_FALSE(event->Deleted());
        });

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagB",
              ItemDescriptor{EvaluationResult{
                  1, std::nullopt, false, false, std::nullopt,
                  EvaluationDetailInternal{Value("potato"), std::nullopt,
                                           std::nullopt}}}}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_TRUE(got_event);
}

TEST(FlagUpdaterDataTests, PatchWithUpdateProducesEvent) {
    FlagStore manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagA",
        [&got_event, &manager](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            EXPECT_EQ("test", event->OldValue().AsString());
            EXPECT_EQ("second", event->NewValue().AsString());
            EXPECT_EQ("flagA", event->FlagName());
            EXPECT_FALSE(event->Deleted());
            // The value in the store should be consistent with the new value.
            EXPECT_EQ(event->NewValue().AsString(), manager.Get("flagA")
                                                        .get()
                                                        ->item.value()
                                                        .Detail()
                                                        .Value()
                                                        .AsString());
        });

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           0, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    updater.Upsert(ContextBuilder().Kind("user", "user-key").Build(), "flagA",
                   ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_TRUE(got_event);
}

TEST(FlagUpdaterEventTests, PatchWithNewFlagProducesEvent) {
    FlagStore manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagB",
        [&got_event, &manager](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            EXPECT_TRUE(event->OldValue().IsNull());
            EXPECT_EQ("second", event->NewValue().AsString());
            EXPECT_EQ("flagB", event->FlagName());
            EXPECT_FALSE(event->Deleted());

            // The value in the store should be consistent with the new value.
            EXPECT_EQ(event->NewValue().AsString(), manager.Get("flagB")
                                                        .get()
                                                        ->item.value()
                                                        .Detail()
                                                        .Value()
                                                        .AsString());
        });

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    updater.Upsert(ContextBuilder().Kind("user", "user-key").Build(), "flagB",
                   ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_TRUE(got_event);
}

TEST(FlagUpdaterEventTests, OutOfOrderPatchProducesNoEvent) {
    FlagStore manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagA", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);
        });

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    updater.Upsert(ContextBuilder().Kind("user", "user-key").Build(), "flagA",
                   ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_FALSE(got_event);
}

TEST(FlagUpdaterEventTests, EqualVersionProducesNoEvent) {
    FlagStore manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagA", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);
        });

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    updater.Upsert(ContextBuilder().Kind("user", "user-key").Build(), "flagA",
                   ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_FALSE(got_event);
}

TEST(FlagUpdaterEventTests, DeleteProducesAnEvent) {
    FlagStore manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagA", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            EXPECT_EQ("test", event->OldValue().AsString());
            EXPECT_TRUE(event->NewValue().IsNull());
            EXPECT_EQ("flagA", event->FlagName());
            EXPECT_TRUE(event->Deleted());
        });

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    updater.Upsert(ContextBuilder().Kind("user", "user-key").Build(), "flagA",
                   ItemDescriptor{Tombstone{2}});

    EXPECT_TRUE(got_event);
}

TEST(FlagUpdaterEventTests, FlagMissingFromSecondInitTreatedAsDelete) {
    FlagStore manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagB", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            EXPECT_EQ("test-b", event->OldValue().AsString());
            EXPECT_TRUE(event->NewValue().IsNull());
            EXPECT_EQ("flagB", event->FlagName());
            EXPECT_TRUE(event->Deleted());
        });

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}},
             {"flagB",
              ItemDescriptor{EvaluationResult{
                  1, std::nullopt, false, false, std::nullopt,
                  EvaluationDetailInternal{Value("test-b"), std::nullopt,
                                           std::nullopt}}}}}});

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    EXPECT_TRUE(got_event);
}

TEST(FlagUpdaterEventTests, InitWithoutEvaluationResultTreatedAsDelete) {
    // This isn't a condition that will happen, but our local data model allows
    // is, so we are testing it.
    FlagStore manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagB", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            EXPECT_EQ("test-b", event->OldValue().AsString());
            EXPECT_TRUE(event->NewValue().IsNull());
            EXPECT_EQ("flagB", event->FlagName());
            EXPECT_TRUE(event->Deleted());
        });

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}},
             {"flagB",
              ItemDescriptor{EvaluationResult{
                  1, std::nullopt, false, false, std::nullopt,
                  EvaluationDetailInternal{Value("test-b"), std::nullopt,
                                           std::nullopt}}}}}});

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}},
             {"flagB", ItemDescriptor{Tombstone{2}}}}});

    EXPECT_TRUE(got_event);
}

TEST(FlagUpdaterEventTests, DeletedFlagStillDeletedInit) {
    FlagStore manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagB", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            EXPECT_EQ("test-b", event->OldValue().AsString());
            EXPECT_TRUE(event->NewValue().IsNull());
            EXPECT_EQ("flagB", event->FlagName());
            EXPECT_TRUE(event->Deleted());
        });

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}},
             {"flagB",
              ItemDescriptor{EvaluationResult{
                  1, std::nullopt, false, false, std::nullopt,
                  EvaluationDetailInternal{Value("test-b"), std::nullopt,
                                           std::nullopt}}}}}});

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    got_event.store(false);

    // Do another init where it is still deleted.
    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    EXPECT_FALSE(got_event);
}

TEST(FlagUpdaterEventTests, SecondDeleteNoEventPatch) {
    FlagStore manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagA", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            EXPECT_EQ("test", event->OldValue().AsString());
            EXPECT_TRUE(event->NewValue().IsNull());
            EXPECT_EQ("flagA", event->FlagName());
            EXPECT_TRUE(event->Deleted());
        });

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    updater.Upsert(ContextBuilder().Kind("user", "user-key").Build(), "flagA",
                   ItemDescriptor{Tombstone{2}});

    got_event.store(false);

    updater.Upsert(ContextBuilder().Kind("user", "user-key").Build(), "flagA",
                   ItemDescriptor{Tombstone{3}});

    EXPECT_FALSE(got_event);
}

TEST(FlagUpdaterDataTests, CanDisconnectEventAndStopGettingEvents) {
    FlagStore manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    auto connection = notifier->OnFlagChange(
        "flagA", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            EXPECT_EQ("test", event->OldValue().AsString());
            EXPECT_EQ("second", event->NewValue().AsString());
            EXPECT_EQ("flagA", event->FlagName());
            EXPECT_FALSE(event->Deleted());
        });

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           0, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    updater.Upsert(ContextBuilder().Kind("user", "user-key").Build(), "flagA",
                   ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    got_event.store(false);
    connection->Disconnect();

    updater.Upsert(ContextBuilder().Kind("user", "user-key").Build(), "flagA",
                   ItemDescriptor{EvaluationResult{
                       2, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("third"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_FALSE(got_event);
}

TEST(FlagUpdaterEventTests, UndeletedFlagProducesEvent) {
    FlagStore manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagA", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            if (event->Deleted()) {
                EXPECT_EQ("test", event->OldValue().AsString());
                EXPECT_TRUE(event->NewValue().IsNull());
                EXPECT_EQ("flagA", event->FlagName());
            } else {
                EXPECT_EQ("second", event->NewValue().AsString());
                EXPECT_TRUE(event->OldValue().IsNull());
                EXPECT_EQ("flagA", event->FlagName());
            }
        });

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{Value("test"), std::nullopt,
                                                    std::nullopt}}}}}});

    updater.Upsert(ContextBuilder().Kind("user", "user-key").Build(), "flagA",
                   ItemDescriptor{Tombstone{2}});

    got_event.store(false);

    updater.Upsert(ContextBuilder().Kind("user", "user-key").Build(), "flagA",
                   ItemDescriptor{EvaluationResult{
                       3, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_TRUE(got_event);
}

TEST(FlagUpdaterEventTests, CanListenToMultipleFlags) {
    FlagStore manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event_a(false);
    std::atomic_bool got_event_b(false);
    notifier->OnFlagChange(
        "flagB",
        [&got_event_b](std::shared_ptr<FlagValueChangeEvent> const& event) {
            got_event_b.store(true);

            EXPECT_EQ("test-b", event->OldValue().AsString());
            EXPECT_EQ("second-b", event->NewValue().AsString());
            EXPECT_EQ("flagB", event->FlagName());
        });

    notifier->OnFlagChange(
        "flagA",
        [&got_event_a](std::shared_ptr<FlagValueChangeEvent> const& event) {
            got_event_a.store(true);

            EXPECT_EQ("test-a", event->OldValue().AsString());
            EXPECT_EQ("second-a", event->NewValue().AsString());
            EXPECT_EQ("flagA", event->FlagName());
        });

    updater.Init(
        ContextBuilder().Kind("user", "user-key").Build(),
        std::unordered_map<std::string, ItemDescriptor>{
            {{"flagA", ItemDescriptor{EvaluationResult{
                           1, std::nullopt, false, false, std::nullopt,
                           EvaluationDetailInternal{
                               Value("test-a"), std::nullopt, std::nullopt}}}},
             {"flagB",
              ItemDescriptor{EvaluationResult{
                  1, std::nullopt, false, false, std::nullopt,
                  EvaluationDetailInternal{Value("test-b"), std::nullopt,
                                           std::nullopt}}}}}});

    updater.Upsert(ContextBuilder().Kind("user", "user-key").Build(), "flagA",
                   ItemDescriptor{EvaluationResult{
                       3, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second-a"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_TRUE(got_event_a);
    EXPECT_FALSE(got_event_b);

    updater.Upsert(ContextBuilder().Kind("user", "user-key").Build(), "flagB",
                   ItemDescriptor{EvaluationResult{
                       3, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second-b"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_TRUE(got_event_a);
    EXPECT_TRUE(got_event_b);
}
