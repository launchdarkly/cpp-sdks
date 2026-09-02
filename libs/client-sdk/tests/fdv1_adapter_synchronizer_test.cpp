#include <gtest/gtest.h>

#include <data_sources/fdv2/fdv1_adapter_synchronizer.hpp>

#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/data/evaluation_detail_internal.hpp>
#include <launchdarkly/data/evaluation_result.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

#include <memory>
#include <string>
#include <utility>

using launchdarkly::ContextBuilder;
using launchdarkly::EvaluationDetailInternal;
using launchdarkly::EvaluationResult;
using launchdarkly::Value;
using launchdarkly::client_side::IDataSourceUpdateSink;
using launchdarkly::client_side::ItemDescriptor;

using namespace launchdarkly::client_side::data_sources;

namespace {

ItemDescriptor Flag(std::uint64_t version, Value value) {
    return ItemDescriptor{
        EvaluationResult{version, std::nullopt, false, false, std::nullopt,
                         EvaluationDetailInternal{std::move(value),
                                                  std::nullopt, std::nullopt}}};
}

// Stands in for an FDv1 data source. Records its lifecycle and lets tests
// drive the sink and status manager it was handed.
class FakeFDv1Source : public IDataSource {
   public:
    FakeFDv1Source(IDataSourceUpdateSink* sink,
                   DataSourceStatusManager* status_manager)
        : sink_(sink), status_manager_(status_manager) {}

    void Start() override { ++start_count_; }

    void ShutdownAsync(std::function<void()> completion) override {
        ++shutdown_count_;
        if (completion) {
            completion();
        }
    }

    IDataSourceUpdateSink* Sink() { return sink_; }
    DataSourceStatusManager* StatusManager() { return status_manager_; }

    int start_count_ = 0;
    int shutdown_count_ = 0;

   private:
    IDataSourceUpdateSink* const sink_;
    DataSourceStatusManager* const status_manager_;
};

// Builds an adapter over a FakeFDv1Source and keeps a handle on it.
class AdapterFixture {
   public:
    AdapterFixture() {
        adapter_ = std::make_unique<FDv1AdapterSynchronizer>(
            [this](IDataSourceUpdateSink* sink,
                   DataSourceStatusManager* status_manager) {
                auto source =
                    std::make_shared<FakeFDv1Source>(sink, status_manager);
                source_ = source;
                return source;
            });
    }

    FDv1AdapterSynchronizer& Adapter() { return *adapter_; }
    FakeFDv1Source& Source() { return *source_; }

    std::optional<FDv2SourceResult> Next() {
        return adapter_->Next(launchdarkly::data_model::Selector{})
            .WaitForResult(std::chrono::seconds{2});
    }

   private:
    std::shared_ptr<FakeFDv1Source> source_;
    std::unique_ptr<FDv1AdapterSynchronizer> adapter_;
};

}  // namespace

TEST(FDv1AdapterSynchronizerTest, TheFirstNextStartsTheWrappedSource) {
    AdapterFixture f;

    EXPECT_EQ(0, f.Source().start_count_);

    f.Source().Sink()->Init(ContextBuilder().Kind("user", "user-key").Build(),
                            {{"flagA", Flag(1, Value("a"))}});
    f.Next();

    EXPECT_EQ(1, f.Source().start_count_);
}

TEST(FDv1AdapterSynchronizerTest, InitBecomesAFullChangeSet) {
    AdapterFixture f;

    f.Source().Sink()->Init(ContextBuilder().Kind("user", "user-key").Build(),
                            {{"flagA", Flag(1, Value("a"))}});
    auto result = f.Next();

    ASSERT_TRUE(result.has_value());
    auto* change_set = std::get_if<FDv2SourceResult::ChangeSet>(&result->value);
    ASSERT_NE(nullptr, change_set);
    EXPECT_EQ(launchdarkly::data_model::ChangeSetType::kFull,
              change_set->change_set.type);
    ASSERT_EQ(1u, change_set->change_set.data.size());
    EXPECT_EQ("flagA", change_set->change_set.data[0].key);
}

TEST(FDv1AdapterSynchronizerTest, UpsertBecomesAPartialChangeSet) {
    AdapterFixture f;

    f.Source().Sink()->Upsert(ContextBuilder().Kind("user", "user-key").Build(),
                              "flagA", Flag(2, Value("a2")));
    auto result = f.Next();

    ASSERT_TRUE(result.has_value());
    auto* change_set = std::get_if<FDv2SourceResult::ChangeSet>(&result->value);
    ASSERT_NE(nullptr, change_set);
    EXPECT_EQ(launchdarkly::data_model::ChangeSetType::kPartial,
              change_set->change_set.type);
    ASSERT_EQ(1u, change_set->change_set.data.size());
    EXPECT_EQ("flagA", change_set->change_set.data[0].key);
}

// FDv1 has no selectors, so the orchestrator must never end up asking the
// service for a delta against data FDv1 supplied.
TEST(FDv1AdapterSynchronizerTest, ChangeSetsCarryNoSelector) {
    AdapterFixture f;

    f.Source().Sink()->Init(ContextBuilder().Kind("user", "user-key").Build(),
                            {{"flagA", Flag(1, Value("a"))}});
    auto result = f.Next();

    auto* change_set = std::get_if<FDv2SourceResult::ChangeSet>(&result->value);
    ASSERT_NE(nullptr, change_set);
    EXPECT_FALSE(change_set->change_set.selector.value.has_value());
}

TEST(FDv1AdapterSynchronizerTest, ARecoverableErrorBecomesInterrupted) {
    AdapterFixture f;

    f.Source().StatusManager()->SetState(
        DataSourceStatus::DataSourceState::kInterrupted,
        DataSourceStatus::ErrorInfo::ErrorKind::kNetworkError, "boom");
    auto result = f.Next();

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Interrupted>(result->value));
}

TEST(FDv1AdapterSynchronizerTest, AnUnrecoverableErrorBecomesTerminal) {
    AdapterFixture f;

    f.Source().StatusManager()->SetState(
        DataSourceStatus::DataSourceState::kShutdown,
        DataSourceStatus::ErrorInfo::ErrorKind::kErrorResponse, "unauthorized");
    auto result = f.Next();

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::TerminalError>(result->value));
}

// The changeset that accompanies a recovery already reports it.
TEST(FDv1AdapterSynchronizerTest, BecomingValidReportsNothing) {
    AdapterFixture f;

    f.Source().StatusManager()->SetState(
        DataSourceStatus::DataSourceState::kValid);

    auto future = f.Adapter().Next(launchdarkly::data_model::Selector{});
    EXPECT_FALSE(future.IsFinished());
}

TEST(FDv1AdapterSynchronizerTest, CloseShutsDownTheWrappedSource) {
    AdapterFixture f;

    f.Source().Sink()->Init(ContextBuilder().Kind("user", "user-key").Build(),
                            {});
    f.Next();
    f.Adapter().Close();

    EXPECT_EQ(1, f.Source().shutdown_count_);
}

// Nothing was started, so there is nothing to shut down.
TEST(FDv1AdapterSynchronizerTest, CloseBeforeAnyNextDoesNotShutDown) {
    AdapterFixture f;

    f.Adapter().Close();

    EXPECT_EQ(0, f.Source().shutdown_count_);
    EXPECT_EQ(0, f.Source().start_count_);
}

TEST(FDv1AdapterSynchronizerTest, NextAfterCloseIsShutdown) {
    AdapterFixture f;

    f.Adapter().Close();
    auto result = f.Next();

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Shutdown>(result->value));
    EXPECT_EQ(0, f.Source().start_count_);
}

TEST(FDv1AdapterSynchronizerTest, CloseUnblocksAPendingNext) {
    AdapterFixture f;

    auto future = f.Adapter().Next(launchdarkly::data_model::Selector{});
    ASSERT_FALSE(future.IsFinished());

    f.Adapter().Close();

    ASSERT_TRUE(future.IsFinished());
    EXPECT_TRUE(std::holds_alternative<FDv2SourceResult::Shutdown>(
        future.GetResult()->value));
}
