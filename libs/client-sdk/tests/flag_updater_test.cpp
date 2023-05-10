#include <gtest/gtest.h>

#include "data/evaluation_detail_internal.hpp"
#include "data/evaluation_result.hpp"
#include "launchdarkly/client_side/data_source_update_sink.hpp"
#include "launchdarkly/client_side/flag_manager/detail/flag_manager.hpp"
#include "launchdarkly/client_side/flag_manager/detail/flag_updater.hpp"

using launchdarkly::EvaluationDetailInternal;
using launchdarkly::EvaluationResult;
using launchdarkly::Value;
using launchdarkly::client_side::ItemDescriptor;
using launchdarkly::client_side::flag_manager::detail::FlagManager;
using launchdarkly::client_side::flag_manager::detail::FlagUpdater;
using launchdarkly::client_side::flag_manager::detail::FlagValueChangeEvent;
using launchdarkly::client_side::flag_manager::detail::IFlagNotifier;

TEST(FlagUpdaterDataTests, HandlesEmptyInit) {
    FlagManager manager;
    FlagUpdater updater(manager);

    updater.Init(
        std::unordered_map<std::string,
                           launchdarkly::client_side::ItemDescriptor>{});

    EXPECT_TRUE(manager.GetAll().empty());
}

TEST(FlagUpdaterDataTests, HandlesInitWithData) {
    FlagManager manager;
    FlagUpdater updater(manager);

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_EQ("test", manager.Get("flagA")->flag.value().detail().value());
}

TEST(FlagUpdaterDataTests, HandlesSecondInit) {
    FlagManager manager;
    FlagUpdater updater(manager);

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagB", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_EQ("test", manager.Get("flagB")->flag.value().detail().value());
    EXPECT_FALSE(manager.Get("flagA"));
}

TEST(FlagUpdaterDataTests, HandlePatchNewFlag) {
    FlagManager manager;
    FlagUpdater updater(manager);

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    updater.Upsert("flagB",
                   ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_EQ("test", manager.Get("flagA")->flag.value().detail().value());
    EXPECT_EQ("second", manager.Get("flagB")->flag.value().detail().value());
}

TEST(FlagUpdaterDataTests, HandlePatchUpdateFlag) {
    FlagManager manager;
    FlagUpdater updater(manager);

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    updater.Upsert("flagA",
                   ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_EQ("second", manager.Get("flagA")->flag.value().detail().value());
}

TEST(FlagUpdaterDataTests, HandlePatchOutOfOrder) {
    FlagManager manager;
    FlagUpdater updater(manager);

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    updater.Upsert("flagA",
                   ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_EQ("test", manager.Get("flagA")->flag.value().detail().value());
}

TEST(FlagUpdaterDataTests, HandleDelete) {
    FlagManager manager;
    FlagUpdater updater(manager);

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    updater.Upsert("flagA", ItemDescriptor{2});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_FALSE(manager.Get("flagA")->flag.has_value());
}

TEST(FlagUpdaterDataTests, HandleDeleteOutOfOrder) {
    FlagManager manager;
    FlagUpdater updater(manager);

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    updater.Upsert("flagA", ItemDescriptor{0});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_EQ("test", manager.Get("flagA")->flag.value().detail().value());
}

TEST(FlagUpdaterEventTests, InitialInitProducesNoEvents) {
    FlagManager manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagA", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);
        });

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_FALSE(got_event);
}

TEST(FlagUpdaterEventTests, SecondInitWithUpdateProducesEvents) {
    FlagManager manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagA", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            EXPECT_EQ("test", event->OldValue().as_string());
            EXPECT_EQ("potato", event->NewValue().as_string());
            EXPECT_EQ("flagA", event->FlagName());
            EXPECT_FALSE(event->Deleted());
        });

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("potato"), std::nullopt,
                                                std::nullopt}}}}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_TRUE(got_event);
}

TEST(FlagUpdaterEventTests, SecondInitWithNewFlagProducesEvents) {
    FlagManager manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagB", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            EXPECT_TRUE(event->OldValue().IsNull());
            EXPECT_EQ("potato", event->NewValue().as_string());
            EXPECT_EQ("flagB", event->FlagName());
            EXPECT_FALSE(event->Deleted());
        });

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagB", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("potato"), std::nullopt,
                                                std::nullopt}}}}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_TRUE(got_event);
}

TEST(FlagUpdaterDataTests, PatchWithUpdateProducesEvent) {
    FlagManager manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagA", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            EXPECT_EQ("test", event->OldValue().as_string());
            EXPECT_EQ("second", event->NewValue().as_string());
            EXPECT_EQ("flagA", event->FlagName());
            EXPECT_FALSE(event->Deleted());
        });

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    updater.Upsert("flagA",
                   ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_TRUE(got_event);
}

TEST(FlagUpdaterEventTests, PatchWithNewFlagProducesEvent) {
    FlagManager manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagB", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            EXPECT_TRUE(event->OldValue().IsNull());
            EXPECT_EQ("second", event->NewValue().as_string());
            EXPECT_EQ("flagB", event->FlagName());
            EXPECT_FALSE(event->Deleted());
        });

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    updater.Upsert("flagB",
                   ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_FALSE(manager.GetAll().empty());
    EXPECT_TRUE(got_event);
}

TEST(FlagUpdaterEventTests, OutOfOrderPatchProducesNoEvent) {
    FlagManager manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagA", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);
        });

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    updater.Upsert("flagA",
                   ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_FALSE(got_event);
}

TEST(FlagUpdaterEventTests, EqualVersionProducesNoEvent) {
    FlagManager manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagA", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);
        });

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    updater.Upsert("flagA",
                   ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_FALSE(got_event);
}

TEST(FlagUpdaterEventTests, DeleteProducesAnEvent) {
    FlagManager manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagA", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            EXPECT_EQ("test", event->OldValue().as_string());
            EXPECT_TRUE(event->NewValue().IsNull());
            EXPECT_EQ("flagA", event->FlagName());
            EXPECT_TRUE(event->Deleted());
        });

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    updater.Upsert("flagA", ItemDescriptor{2});

    EXPECT_TRUE(got_event);
}

TEST(FlagUpdaterEventTests, FlagMissingFromSecondInitTreatedAsDelete) {
    FlagManager manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagB", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            EXPECT_EQ("test-b", event->OldValue().as_string());
            EXPECT_TRUE(event->NewValue().IsNull());
            EXPECT_EQ("flagB", event->FlagName());
            EXPECT_TRUE(event->Deleted());
        });

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}},
         {"flagB", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test-b"), std::nullopt,
                                                std::nullopt}}}}}});

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    EXPECT_TRUE(got_event);
}

TEST(FlagUpdaterEventTests, InitWithoutEvaluationResultTreatedAsDelete) {
    // This isn't a condition that will happen, but our local data model allows
    // is, so we are testing it.
    FlagManager manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagB", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            EXPECT_EQ("test-b", event->OldValue().as_string());
            EXPECT_TRUE(event->NewValue().IsNull());
            EXPECT_EQ("flagB", event->FlagName());
            EXPECT_TRUE(event->Deleted());
        });

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}},
         {"flagB", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test-b"), std::nullopt,
                                                std::nullopt}}}}}});

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}},
         {"flagB", ItemDescriptor{2}}}});

    EXPECT_TRUE(got_event);
}

TEST(FlagUpdaterEventTests, DeletedFlagStillDeletedInit) {
    FlagManager manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagB", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            EXPECT_EQ("test-b", event->OldValue().as_string());
            EXPECT_TRUE(event->NewValue().IsNull());
            EXPECT_EQ("flagB", event->FlagName());
            EXPECT_TRUE(event->Deleted());
        });

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}},
         {"flagB", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test-b"), std::nullopt,
                                                std::nullopt}}}}}});

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    got_event.store(false);

    // Do another init where it is still deleted.
    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    EXPECT_FALSE(got_event);
}

TEST(FlagUpdaterEventTests, SecondDeleteNoEventPatch) {
    FlagManager manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagA", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            EXPECT_EQ("test", event->OldValue().as_string());
            EXPECT_TRUE(event->NewValue().IsNull());
            EXPECT_EQ("flagA", event->FlagName());
            EXPECT_TRUE(event->Deleted());
        });

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    updater.Upsert("flagA", ItemDescriptor{2});

    got_event.store(false);

    updater.Upsert("flagA", ItemDescriptor{3});

    EXPECT_FALSE(got_event);
}

TEST(FlagUpdaterDataTests, CanDisconnectEventAndStopGettingEvents) {
    FlagManager manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    auto connection = notifier->OnFlagChange(
        "flagA", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            EXPECT_EQ("test", event->OldValue().as_string());
            EXPECT_EQ("second", event->NewValue().as_string());
            EXPECT_EQ("flagA", event->FlagName());
            EXPECT_FALSE(event->Deleted());
        });

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       0, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    updater.Upsert("flagA",
                   ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    got_event.store(false);
    connection->Disconnect();

    updater.Upsert("flagA",
                   ItemDescriptor{EvaluationResult{
                       2, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("third"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_FALSE(got_event);
}

TEST(FlagUpdaterEventTests, UndeletedFlagProducesEvent) {
    FlagManager manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event(false);
    notifier->OnFlagChange(
        "flagA", [&got_event](std::shared_ptr<FlagValueChangeEvent> event) {
            got_event.store(true);

            if (event->Deleted()) {
                EXPECT_EQ("test", event->OldValue().as_string());
                EXPECT_TRUE(event->NewValue().IsNull());
                EXPECT_EQ("flagA", event->FlagName());
            } else {
                EXPECT_EQ("second", event->NewValue().as_string());
                EXPECT_TRUE(event->OldValue().IsNull());
                EXPECT_EQ("flagA", event->FlagName());
            }
        });

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test"), std::nullopt,
                                                std::nullopt}}}}}});

    updater.Upsert("flagA", ItemDescriptor{2});

    got_event.store(false);

    updater.Upsert("flagA",
                   ItemDescriptor{EvaluationResult{
                       3, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_TRUE(got_event);
}

TEST(FlagUpdaterEventTests, CanListenToMultipleFlags) {
    FlagManager manager;
    FlagUpdater updater(manager);

    IFlagNotifier* notifier = &updater;

    std::atomic_bool got_event_a(false);
    std::atomic_bool got_event_b(false);
    notifier->OnFlagChange(
        "flagB",
        [&got_event_b](std::shared_ptr<FlagValueChangeEvent> const& event) {
            got_event_b.store(true);

            EXPECT_EQ("test-b", event->OldValue().as_string());
            EXPECT_EQ("second-b", event->NewValue().as_string());
            EXPECT_EQ("flagB", event->FlagName());
        });

    notifier->OnFlagChange(
        "flagA",
        [&got_event_a](std::shared_ptr<FlagValueChangeEvent> const& event) {
            got_event_a.store(true);

            EXPECT_EQ("test-a", event->OldValue().as_string());
            EXPECT_EQ("second-a", event->NewValue().as_string());
            EXPECT_EQ("flagA", event->FlagName());
        });

    updater.Init(std::unordered_map<std::string,
                                    launchdarkly::client_side::ItemDescriptor>{
        {{"flagA", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test-a"), std::nullopt,
                                                std::nullopt}}}},
         {"flagB", ItemDescriptor{EvaluationResult{
                       1, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("test-b"), std::nullopt,
                                                std::nullopt}}}}}});

    updater.Upsert("flagA",
                   ItemDescriptor{EvaluationResult{
                       3, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second-a"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_TRUE(got_event_a);
    EXPECT_FALSE(got_event_b);

    updater.Upsert("flagB",
                   ItemDescriptor{EvaluationResult{
                       3, std::nullopt, false, false, std::nullopt,
                       EvaluationDetailInternal{Value("second-b"), std::nullopt,
                                                std::nullopt}}});

    EXPECT_TRUE(got_event_a);
    EXPECT_TRUE(got_event_b);
}
