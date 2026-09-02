#include <gtest/gtest.h>

#include <data_sources/fdv2/fdv2_data_source.hpp>
#include <flag_manager/flag_manager.hpp>

#include <launchdarkly/async/promise.hpp>
#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/data/evaluation_detail_internal.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/logging/null_logger.hpp>

#include <boost/asio/io_context.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace launchdarkly;
using namespace launchdarkly::client_side;
using namespace launchdarkly::client_side::data_sources;
using namespace std::chrono_literals;

namespace {

Logger MakeNullLogger() {
    struct NullBackend : ILogBackend {
        bool Enabled(LogLevel) noexcept override { return false; }
        void Write(LogLevel, std::string) noexcept override {}
    };
    return Logger{std::make_shared<NullBackend>()};
}

// Initializer that resolves Run() with a single pre-set result.
class MockInitializer : public IFDv2Initializer {
   public:
    explicit MockInitializer(FDv2SourceResult result,
                             bool* closed_flag = nullptr)
        : result_(std::move(result)), closed_flag_(closed_flag) {}

    async::Future<FDv2SourceResult> Run() override {
        return async::MakeFuture(std::move(result_));
    }

    void Close() override {
        if (closed_flag_) {
            *closed_flag_ = true;
        }
    }

    std::string const& Identity() const override {
        static std::string const id = "mock initializer";
        return id;
    }

   private:
    FDv2SourceResult result_;
    bool* closed_flag_;
};

// Synchronizer that resolves successive Next() calls from a queue of results.
// Once the queue is exhausted it returns Shutdown to end orchestration, unless
// stall_after_results is set, in which case the next Future never resolves.
class MockSynchronizer : public IFDv2Synchronizer {
   public:
    MockSynchronizer(std::vector<FDv2SourceResult> results,
                     bool* closed_flag = nullptr,
                     std::vector<data_model::Selector>* next_calls = nullptr,
                     bool stall_after_results = false)
        : results_(std::move(results)),
          closed_flag_(closed_flag),
          next_calls_(next_calls),
          stall_after_results_(stall_after_results) {}

    async::Future<FDv2SourceResult> Next(
        data_model::Selector selector) override {
        if (next_calls_) {
            next_calls_->push_back(selector);
        }
        if (call_index_ < results_.size()) {
            return async::MakeFuture(std::move(results_[call_index_++]));
        }
        if (stall_after_results_) {
            return stall_promise_.GetFuture();
        }
        return async::MakeFuture(
            FDv2SourceResult{FDv2SourceResult::Shutdown{}});
    }

    void Close() override {
        stall_promise_.Resolve(FDv2SourceResult{FDv2SourceResult::Shutdown{}});
        if (closed_flag_) {
            *closed_flag_ = true;
        }
    }

    std::string const& Identity() const override {
        static std::string const id = "mock synchronizer";
        return id;
    }

   private:
    std::vector<FDv2SourceResult> results_;
    std::size_t call_index_ = 0;
    bool* closed_flag_;
    std::vector<data_model::Selector>* next_calls_;
    bool stall_after_results_;
    async::Promise<FDv2SourceResult> stall_promise_;
};

// Returns a pre-supplied source on its first Build() call.
class OneShotInitializerFactory : public IFDv2InitializerFactory {
   public:
    explicit OneShotInitializerFactory(std::unique_ptr<IFDv2Initializer> source,
                                       bool from_cache = false)
        : source_(std::move(source)), from_cache_(from_cache) {}

    std::unique_ptr<IFDv2Initializer> Build() override {
        ++build_count_;
        return std::move(source_);
    }

    [[nodiscard]] bool IsFromCache() const override { return from_cache_; }

    int build_count_ = 0;

   private:
    std::unique_ptr<IFDv2Initializer> source_;
    bool from_cache_;
};

class OneShotSynchronizerFactory : public IFDv2SynchronizerFactory {
   public:
    explicit OneShotSynchronizerFactory(
        std::unique_ptr<IFDv2Synchronizer> source)
        : source_(std::move(source)) {}

    std::unique_ptr<IFDv2Synchronizer> Build() override {
        ++build_count_;
        return std::move(source_);
    }

    int build_count_ = 0;

   private:
    std::unique_ptr<IFDv2Synchronizer> source_;
};

// Returns each pre-supplied source in order on successive Build() calls, so
// that a factory reused by recovery can hand out a fresh source.
class MultiShotSynchronizerFactory : public IFDv2SynchronizerFactory {
   public:
    explicit MultiShotSynchronizerFactory(
        std::vector<std::unique_ptr<IFDv2Synchronizer>> sources)
        : sources_(std::move(sources)) {}

    std::unique_ptr<IFDv2Synchronizer> Build() override {
        ++build_count_;
        if (build_count_ <= static_cast<int>(sources_.size())) {
            return std::move(sources_[build_count_ - 1]);
        }
        return nullptr;
    }

    int build_count_ = 0;

   private:
    std::vector<std::unique_ptr<IFDv2Synchronizer>> sources_;
};

// Initializer whose Run() never resolves, so that orchestration can be
// examined while it is in flight.
class StalledInitializer : public IFDv2Initializer {
   public:
    explicit StalledInitializer(bool* closed_flag)
        : closed_flag_(closed_flag) {}

    async::Future<FDv2SourceResult> Run() override {
        return promise_.GetFuture();
    }

    void Close() override {
        if (closed_flag_) {
            *closed_flag_ = true;
        }
    }

    std::string const& Identity() const override {
        static std::string const id = "stalled initializer";
        return id;
    }

   private:
    async::Promise<FDv2SourceResult> promise_;
    bool* closed_flag_;
};

data_model::Selector MakeSelector(std::int64_t version, std::string state) {
    return data_model::Selector{
        data_model::Selector::State{version, std::move(state)}};
}

ItemDescriptor MakeFlag(std::uint64_t version, Value value) {
    return ItemDescriptor{
        EvaluationResult{version, std::nullopt, false, false, std::nullopt,
                         EvaluationDetailInternal{std::move(value),
                                                  std::nullopt, std::nullopt}}};
}

FDv2SourceResult MakeChangeSetResult(data_model::ChangeSetType type,
                                     FlagChangeSetData data,
                                     data_model::Selector selector) {
    return FDv2SourceResult{FDv2SourceResult::ChangeSet{
        FlagChangeSet{type, std::move(data), std::move(selector)}}};
}

FDv2SourceResult MakeErrorResult(FDv2SourceResult::Value value) {
    return FDv2SourceResult{std::move(value)};
}

FDv2SourceResult::ErrorInfo MakeError(std::string message) {
    return FDv2SourceResult::ErrorInfo{
        FDv2SourceResult::ErrorInfo::ErrorKind::kNetworkError, 0,
        std::move(message), std::chrono::system_clock::now()};
}

// Records the from_cache flag of each apply before passing it along, so tests
// can assert how the data source classified its sources.
class RecordingSink : public IDataSourceUpdateSink {
   public:
    RecordingSink(IDataSourceUpdateSink* inner, std::vector<bool>* applies)
        : inner_(inner), applies_(applies) {}

    void Init(Context const& context,
              std::unordered_map<std::string, ItemDescriptor> data) override {
        inner_->Init(context, std::move(data));
    }

    void Upsert(Context const& context,
                std::string key,
                ItemDescriptor item) override {
        inner_->Upsert(context, std::move(key), std::move(item));
    }

    void Apply(Context const& context,
               FlagChangeSet change_set,
               bool from_cache) override {
        applies_->push_back(from_cache);
        inner_->Apply(context, std::move(change_set), from_cache);
    }

   private:
    IDataSourceUpdateSink* const inner_;
    std::vector<bool>* const applies_;
};

// Owns everything a data source needs to run against a real flag store.
class Harness {
   public:
    Harness()
        : flag_manager_("sdk-key", logger_, 5, nullptr),
          sink_(&flag_manager_.Updater(), &applies_) {}

    std::shared_ptr<FDv2DataSource> MakeDataSource(
        std::vector<std::unique_ptr<IFDv2InitializerFactory>> initializers,
        std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> synchronizers,
        std::unique_ptr<IFDv2ConditionFactory> fallback = nullptr,
        std::unique_ptr<IFDv2ConditionFactory> recovery = nullptr) {
        return std::make_shared<FDv2DataSource>(
            std::move(initializers), std::move(synchronizers),
            std::move(fallback), std::move(recovery), ioc_.get_executor(),
            ContextBuilder().Kind("user", "user-key").Build(), &sink_,
            &flag_manager_.Store(), &status_manager_, logger_);
    }

    boost::asio::io_context& Context() { return ioc_; }
    DataSourceStatusManager& StatusManager() { return status_manager_; }
    flag_manager::FlagStore const& Store() { return flag_manager_.Store(); }

    DataSourceStatus::DataSourceState State() {
        return status_manager_.Status().State();
    }

    std::vector<bool> const& Applies() const { return applies_; }

   private:
    Logger logger_ = MakeNullLogger();
    boost::asio::io_context ioc_;
    DataSourceStatusManager status_manager_;
    flag_manager::FlagManager flag_manager_;
    std::vector<bool> applies_;
    RecordingSink sink_;
};

}  // namespace

// ============================================================================
// Lifecycle
// ============================================================================

TEST(ClientFDv2DataSourceTest, NoSourcesConfiguredIsImmediatelyValid) {
    Harness h;
    auto source = h.MakeDataSource({}, {});

    source->Start();

    EXPECT_EQ(DataSourceStatus::DataSourceState::kValid, h.State());
}

// The client drops a data source as soon as its replacement starts, so a
// status transition from the old one would land on top of the new one's.
TEST(ClientFDv2DataSourceTest, NoStatusIsReportedAfterShutdown) {
    Harness h;

    std::vector<std::unique_ptr<IFDv2InitializerFactory>> initializers;
    initializers.push_back(std::make_unique<OneShotInitializerFactory>(
        std::make_unique<MockInitializer>(MakeChangeSetResult(
            data_model::ChangeSetType::kNone, {}, data_model::Selector{}))));

    auto source = h.MakeDataSource(std::move(initializers), {});
    source->Start();

    bool completed = false;
    source->ShutdownAsync([&completed]() { completed = true; });
    h.StatusManager().SetState(DataSourceStatus::DataSourceState::kValid);

    // Whatever the abandoned orchestration had queued must not overwrite the
    // state the caller sees after the shutdown.
    h.Context().poll();

    EXPECT_TRUE(completed);
    EXPECT_EQ(DataSourceStatus::DataSourceState::kValid, h.State());
}

TEST(ClientFDv2DataSourceTest, ShutdownClosesTheActiveInitializer) {
    Harness h;
    bool closed = false;

    std::vector<std::unique_ptr<IFDv2InitializerFactory>> initializers;
    initializers.push_back(std::make_unique<OneShotInitializerFactory>(
        std::make_unique<StalledInitializer>(&closed)));

    auto source = h.MakeDataSource(std::move(initializers), {});
    source->Start();
    h.Context().run();

    bool completed = false;
    source->ShutdownAsync([&completed] { completed = true; });
    h.Context().restart();
    h.Context().run();

    EXPECT_TRUE(closed);
    EXPECT_TRUE(completed);
}

// ============================================================================
// Initializer phase
// ============================================================================

TEST(ClientFDv2DataSourceTest, InitializerWithABasisAppliesAndBecomesValid) {
    Harness h;

    std::vector<std::unique_ptr<IFDv2InitializerFactory>> initializers;
    initializers.push_back(std::make_unique<OneShotInitializerFactory>(
        std::make_unique<MockInitializer>(
            MakeChangeSetResult(data_model::ChangeSetType::kFull,
                                {FlagChange{"flagA", MakeFlag(1, Value("a"))}},
                                MakeSelector(1, "state-1")))));

    auto source = h.MakeDataSource(std::move(initializers), {});
    source->Start();
    h.Context().run();

    EXPECT_EQ(DataSourceStatus::DataSourceState::kValid, h.State());
    ASSERT_TRUE(h.Store().Get("flagA"));
    EXPECT_EQ(Value("a"), h.Store().Get("flagA")->item->Detail().Value());
    ASSERT_TRUE(h.Store().CurrentSelector().value.has_value());
    EXPECT_EQ("state-1", h.Store().CurrentSelector().value->state);
}

// An initializer that supplies data without a selector has not established a
// basis, so the chain keeps going to find one.
TEST(ClientFDv2DataSourceTest, DataWithoutASelectorContinuesTheChain) {
    Harness h;

    std::vector<std::unique_ptr<IFDv2InitializerFactory>> initializers;
    initializers.push_back(std::make_unique<OneShotInitializerFactory>(
        std::make_unique<MockInitializer>(MakeChangeSetResult(
            data_model::ChangeSetType::kFull,
            {FlagChange{"cached", MakeFlag(1, Value("from-cache"))}},
            data_model::Selector{}))));
    auto second = std::make_unique<OneShotInitializerFactory>(
        std::make_unique<MockInitializer>(MakeChangeSetResult(
            data_model::ChangeSetType::kFull,
            {FlagChange{"live", MakeFlag(1, Value("from-network"))}},
            MakeSelector(1, "state-1"))));
    auto* second_ptr = second.get();
    initializers.push_back(std::move(second));

    auto source = h.MakeDataSource(std::move(initializers), {});
    source->Start();
    h.Context().run();

    EXPECT_EQ(1, second_ptr->build_count_);
    EXPECT_FALSE(h.Store().Get("cached"));
    ASSERT_TRUE(h.Store().Get("live"));
}

TEST(ClientFDv2DataSourceTest, FailedInitializerAdvancesToTheNext) {
    Harness h;

    std::vector<std::unique_ptr<IFDv2InitializerFactory>> initializers;
    initializers.push_back(std::make_unique<OneShotInitializerFactory>(
        std::make_unique<MockInitializer>(MakeErrorResult(
            FDv2SourceResult::Interrupted{MakeError("boom")}))));
    initializers.push_back(std::make_unique<OneShotInitializerFactory>(
        std::make_unique<MockInitializer>(
            MakeChangeSetResult(data_model::ChangeSetType::kFull,
                                {FlagChange{"flagA", MakeFlag(1, Value("a"))}},
                                MakeSelector(1, "state-1")))));

    auto source = h.MakeDataSource(std::move(initializers), {});
    source->Start();
    h.Context().run();

    EXPECT_EQ(DataSourceStatus::DataSourceState::kValid, h.State());
    EXPECT_TRUE(h.Store().Get("flagA"));
}

TEST(ClientFDv2DataSourceTest, ExhaustedInitializersWithNoDataShutDown) {
    Harness h;

    std::vector<std::unique_ptr<IFDv2InitializerFactory>> initializers;
    initializers.push_back(std::make_unique<OneShotInitializerFactory>(
        std::make_unique<MockInitializer>(MakeErrorResult(
            FDv2SourceResult::TerminalError{MakeError("boom")}))));

    auto source = h.MakeDataSource(std::move(initializers), {});
    source->Start();
    h.Context().run();

    EXPECT_EQ(DataSourceStatus::DataSourceState::kShutdown, h.State());
}

// In offline mode the cache is the only thing that could ever supply data,
// so a miss means zero flags rather than a failure to start.
// Under FDv1 the client loaded the cache in its constructor, so cached flags
// were evaluable immediately. They still are.
TEST(ClientFDv2DataSourceTest, CachedDataIsAppliedBeforeStartReturns) {
    Harness h;

    std::vector<std::unique_ptr<IFDv2InitializerFactory>> initializers;
    initializers.push_back(std::make_unique<OneShotInitializerFactory>(
        std::make_unique<MockInitializer>(MakeChangeSetResult(
            data_model::ChangeSetType::kFull,
            {FlagChange{"flagA", MakeFlag(1, Value("cached"))}},
            data_model::Selector{})),
        /* from_cache= */ true));

    auto source = h.MakeDataSource(std::move(initializers), {});
    source->Start();

    // The executor has not run, so only the inline cache pass can have
    // applied anything.
    auto const flag = h.Store().Get("flagA");
    ASSERT_TRUE(flag);
    EXPECT_EQ(Value("cached"), flag->item->Detail().Value());
    EXPECT_EQ(DataSourceStatus::DataSourceState::kValid, h.State());
}

TEST(ClientFDv2DataSourceTest, CacheOnlyModeIsValidEvenOnAMiss) {
    Harness h;

    std::vector<std::unique_ptr<IFDv2InitializerFactory>> initializers;
    initializers.push_back(std::make_unique<OneShotInitializerFactory>(
        std::make_unique<MockInitializer>(MakeChangeSetResult(
            data_model::ChangeSetType::kNone, {}, data_model::Selector{})),
        /* from_cache= */ true));

    auto source = h.MakeDataSource(std::move(initializers), {});
    source->Start();
    h.Context().run();

    EXPECT_EQ(DataSourceStatus::DataSourceState::kValid, h.State());
    EXPECT_TRUE(h.Store().GetAll().empty());
}

// A non-cache initializer returning "none" must not make initialization
// succeed when nothing else can supply data.
TEST(ClientFDv2DataSourceTest, NetworkOnlyNoneResultDoesNotCountAsSuccess) {
    Harness h;

    std::vector<std::unique_ptr<IFDv2InitializerFactory>> initializers;
    initializers.push_back(std::make_unique<OneShotInitializerFactory>(
        std::make_unique<MockInitializer>(MakeChangeSetResult(
            data_model::ChangeSetType::kNone, {}, data_model::Selector{}))));

    auto source = h.MakeDataSource(std::move(initializers), {});
    source->Start();
    h.Context().run();

    EXPECT_EQ(DataSourceStatus::DataSourceState::kShutdown, h.State());
}

// ============================================================================
// Synchronizer phase
// ============================================================================

TEST(ClientFDv2DataSourceTest, SynchronizerChangeSetsAreApplied) {
    Harness h;

    std::vector<FDv2SourceResult> results;
    results.push_back(
        MakeChangeSetResult(data_model::ChangeSetType::kFull,
                            {FlagChange{"flagA", MakeFlag(1, Value("a"))}},
                            MakeSelector(1, "state-1")));
    results.push_back(
        MakeChangeSetResult(data_model::ChangeSetType::kPartial,
                            {FlagChange{"flagA", MakeFlag(2, Value("a2"))}},
                            MakeSelector(2, "state-2")));

    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> synchronizers;
    synchronizers.push_back(std::make_unique<OneShotSynchronizerFactory>(
        std::make_unique<MockSynchronizer>(std::move(results))));

    auto source = h.MakeDataSource({}, std::move(synchronizers));
    source->Start();
    h.Context().run();

    ASSERT_TRUE(h.Store().Get("flagA"));
    EXPECT_EQ(Value("a2"), h.Store().Get("flagA")->item->Detail().Value());
}

// The synchronizer asks the service for changes since the data the store
// already holds.
TEST(ClientFDv2DataSourceTest, SynchronizerReceivesTheStoresSelector) {
    Harness h;
    std::vector<data_model::Selector> next_calls;

    std::vector<FDv2SourceResult> results;
    results.push_back(
        MakeChangeSetResult(data_model::ChangeSetType::kFull,
                            {FlagChange{"flagA", MakeFlag(1, Value("a"))}},
                            MakeSelector(1, "state-1")));

    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> synchronizers;
    synchronizers.push_back(std::make_unique<OneShotSynchronizerFactory>(
        std::make_unique<MockSynchronizer>(std::move(results), nullptr,
                                           &next_calls)));

    auto source = h.MakeDataSource({}, std::move(synchronizers));
    source->Start();
    h.Context().run();

    ASSERT_GE(next_calls.size(), 2u);
    EXPECT_FALSE(next_calls[0].value.has_value());
    ASSERT_TRUE(next_calls[1].value.has_value());
    EXPECT_EQ("state-1", next_calls[1].value->state);
}

TEST(ClientFDv2DataSourceTest, InterruptedSynchronizerKeepsRunning) {
    Harness h;

    std::vector<FDv2SourceResult> results;
    results.push_back(
        MakeErrorResult(FDv2SourceResult::Interrupted{MakeError("boom")}));
    results.push_back(
        MakeChangeSetResult(data_model::ChangeSetType::kFull,
                            {FlagChange{"flagA", MakeFlag(1, Value("a"))}},
                            MakeSelector(1, "state-1")));

    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> synchronizers;
    synchronizers.push_back(std::make_unique<OneShotSynchronizerFactory>(
        std::make_unique<MockSynchronizer>(std::move(results))));

    auto source = h.MakeDataSource({}, std::move(synchronizers));
    source->Start();
    h.Context().run();

    EXPECT_EQ(DataSourceStatus::DataSourceState::kValid, h.State());
    EXPECT_TRUE(h.Store().Get("flagA"));
}

TEST(ClientFDv2DataSourceTest, TerminalErrorAdvancesToTheNextSynchronizer) {
    Harness h;

    std::vector<FDv2SourceResult> first_results;
    first_results.push_back(
        MakeErrorResult(FDv2SourceResult::TerminalError{MakeError("gone")}));

    std::vector<FDv2SourceResult> second_results;
    second_results.push_back(
        MakeChangeSetResult(data_model::ChangeSetType::kFull,
                            {FlagChange{"flagA", MakeFlag(1, Value("a"))}},
                            MakeSelector(1, "state-1")));

    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> synchronizers;
    synchronizers.push_back(std::make_unique<OneShotSynchronizerFactory>(
        std::make_unique<MockSynchronizer>(std::move(first_results))));
    auto second = std::make_unique<OneShotSynchronizerFactory>(
        std::make_unique<MockSynchronizer>(std::move(second_results)));
    auto* second_ptr = second.get();
    synchronizers.push_back(std::move(second));

    auto source = h.MakeDataSource({}, std::move(synchronizers));
    source->Start();
    h.Context().run();

    EXPECT_EQ(1, second_ptr->build_count_);
    EXPECT_TRUE(h.Store().Get("flagA"));
}

TEST(ClientFDv2DataSourceTest, ExhaustedSynchronizersShutDown) {
    Harness h;

    std::vector<FDv2SourceResult> results;
    results.push_back(
        MakeErrorResult(FDv2SourceResult::TerminalError{MakeError("gone")}));

    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> synchronizers;
    synchronizers.push_back(std::make_unique<OneShotSynchronizerFactory>(
        std::make_unique<MockSynchronizer>(std::move(results))));

    auto source = h.MakeDataSource({}, std::move(synchronizers));
    source->Start();
    h.Context().run();

    EXPECT_EQ(DataSourceStatus::DataSourceState::kShutdown, h.State());
}

// ============================================================================
// Environment ID
// ============================================================================

TEST(ClientFDv2DataSourceTest, RecordsTheEnvironmentIdFromAResult) {
    Harness h;

    auto result =
        MakeChangeSetResult(data_model::ChangeSetType::kFull,
                            {FlagChange{"flagA", MakeFlag(1, Value("a"))}},
                            MakeSelector(1, "state-1"));
    result.environment_id = "env-1234";

    std::vector<std::unique_ptr<IFDv2InitializerFactory>> initializers;
    initializers.push_back(std::make_unique<OneShotInitializerFactory>(
        std::make_unique<MockInitializer>(std::move(result))));

    auto source = h.MakeDataSource(std::move(initializers), {});
    source->Start();
    h.Context().run();

    ASSERT_TRUE(source->EnvironmentId().has_value());
    EXPECT_EQ("env-1234", *source->EnvironmentId());
}

TEST(ClientFDv2DataSourceTest, ReportsNoEnvironmentIdUntilOneArrives) {
    Harness h;
    auto source = h.MakeDataSource({}, {});

    source->Start();

    EXPECT_FALSE(source->EnvironmentId().has_value());
}

// ============================================================================
// Fallback and recovery
// ============================================================================

TEST(ClientFDv2DataSourceTest, SustainedInterruptionFallsBackToTheNextTier) {
    Harness h;

    std::vector<FDv2SourceResult> first_results;
    first_results.push_back(
        MakeErrorResult(FDv2SourceResult::Interrupted{MakeError("flaky")}));

    std::vector<FDv2SourceResult> second_results;
    second_results.push_back(
        MakeChangeSetResult(data_model::ChangeSetType::kFull,
                            {FlagChange{"flagA", MakeFlag(1, Value("a"))}},
                            MakeSelector(1, "state-1")));

    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> synchronizers;
    synchronizers.push_back(std::make_unique<OneShotSynchronizerFactory>(
        std::make_unique<MockSynchronizer>(std::move(first_results), nullptr,
                                           nullptr,
                                           /* stall_after_results= */ true)));
    auto second = std::make_unique<OneShotSynchronizerFactory>(
        std::make_unique<MockSynchronizer>(std::move(second_results)));
    auto* second_ptr = second.get();
    synchronizers.push_back(std::move(second));

    auto source = h.MakeDataSource({}, std::move(synchronizers),
                                   std::make_unique<FallbackConditionFactory>(
                                       h.Context().get_executor(), 50ms));
    source->Start();
    h.Context().run();

    EXPECT_EQ(1, second_ptr->build_count_);
    EXPECT_TRUE(h.Store().Get("flagA"));
}

TEST(ClientFDv2DataSourceTest, RecoveryReturnsToThePreferredTier) {
    Harness h;

    // Interrupt, then stall so that the fallback condition wins the race.
    std::vector<FDv2SourceResult> first_attempt;
    first_attempt.push_back(
        MakeErrorResult(FDv2SourceResult::Interrupted{MakeError("flaky")}));

    std::vector<std::unique_ptr<IFDv2Synchronizer>> preferred_sources;
    preferred_sources.push_back(std::make_unique<MockSynchronizer>(
        std::move(first_attempt), nullptr, nullptr,
        /* stall_after_results= */ true));
    preferred_sources.push_back(
        std::make_unique<MockSynchronizer>(std::vector<FDv2SourceResult>{}));

    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> synchronizers;
    auto preferred = std::make_unique<MultiShotSynchronizerFactory>(
        std::move(preferred_sources));
    auto* preferred_ptr = preferred.get();
    synchronizers.push_back(std::move(preferred));
    synchronizers.push_back(std::make_unique<OneShotSynchronizerFactory>(
        std::make_unique<MockSynchronizer>(std::vector<FDv2SourceResult>{},
                                           nullptr, nullptr,
                                           /* stall_after_results= */ true)));

    auto source = h.MakeDataSource({}, std::move(synchronizers),
                                   std::make_unique<FallbackConditionFactory>(
                                       h.Context().get_executor(), 50ms),
                                   std::make_unique<RecoveryConditionFactory>(
                                       h.Context().get_executor(), 50ms));
    source->Start();
    h.Context().run();

    EXPECT_EQ(2, preferred_ptr->build_count_);
}

// ============================================================================
// Cache-sourced data
// ============================================================================

// The store needs to know which data came from the cache, so that it is not
// written straight back to the cache it was read from.
TEST(ClientFDv2DataSourceTest, CacheSourcedDataIsMarkedAsSuch) {
    Harness h;

    std::vector<std::unique_ptr<IFDv2InitializerFactory>> initializers;
    initializers.push_back(std::make_unique<OneShotInitializerFactory>(
        std::make_unique<MockInitializer>(
            MakeChangeSetResult(data_model::ChangeSetType::kFull,
                                {FlagChange{"cached", MakeFlag(1, Value("a"))}},
                                data_model::Selector{})),
        /* from_cache= */ true));
    initializers.push_back(std::make_unique<OneShotInitializerFactory>(
        std::make_unique<MockInitializer>(
            MakeChangeSetResult(data_model::ChangeSetType::kFull,
                                {FlagChange{"live", MakeFlag(1, Value("b"))}},
                                MakeSelector(1, "state-1")))));

    auto source = h.MakeDataSource(std::move(initializers), {});
    source->Start();
    h.Context().run();

    ASSERT_EQ(2u, h.Applies().size());
    EXPECT_TRUE(h.Applies()[0]);
    EXPECT_FALSE(h.Applies()[1]);
}
