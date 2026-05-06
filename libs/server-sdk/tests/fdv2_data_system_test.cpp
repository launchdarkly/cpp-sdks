#include <gtest/gtest.h>

#include <data_components/status_notifications/data_source_status_manager.hpp>
#include <data_interfaces/source/ifdv2_initializer.hpp>
#include <data_interfaces/source/ifdv2_initializer_factory.hpp>
#include <data_interfaces/source/ifdv2_synchronizer.hpp>
#include <data_interfaces/source/ifdv2_synchronizer_factory.hpp>
#include <data_systems/fdv2/fdv2_data_system.hpp>

#include <launchdarkly/async/promise.hpp>
#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/segment.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/server_side/data_source_status.hpp>

#include <boost/asio/io_context.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace launchdarkly;
using namespace launchdarkly::server_side;
using namespace launchdarkly::server_side::data_interfaces;
using namespace launchdarkly::server_side::data_systems;
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
    MockInitializer(FDv2SourceResult result, bool* closed_flag = nullptr)
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
// Once the queue is exhausted, returns Shutdown to terminate orchestration.
class MockSynchronizer : public IFDv2Synchronizer {
   public:
    using NextCall = std::pair<std::chrono::milliseconds, data_model::Selector>;

    MockSynchronizer(std::vector<FDv2SourceResult> results,
                     bool* closed_flag = nullptr,
                     std::vector<NextCall>* next_calls = nullptr)
        : results_(std::move(results)),
          closed_flag_(closed_flag),
          next_calls_(next_calls) {}

    async::Future<FDv2SourceResult> Next(
        std::chrono::milliseconds timeout,
        data_model::Selector selector) override {
        if (next_calls_) {
            next_calls_->push_back({timeout, selector});
        }
        if (call_index_ < results_.size()) {
            return async::MakeFuture(std::move(results_[call_index_++]));
        }
        return async::MakeFuture(
            FDv2SourceResult{FDv2SourceResult::Shutdown{}});
    }

    void Close() override {
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
    std::vector<NextCall>* next_calls_;
};

// One-shot factory: returns a pre-supplied source on its first Build() call.
// Tracks build_count_ so tests can assert whether the factory was invoked.
class OneShotInitializerFactory : public IFDv2InitializerFactory {
   public:
    explicit OneShotInitializerFactory(std::unique_ptr<IFDv2Initializer> source)
        : source_(std::move(source)) {}

    std::unique_ptr<IFDv2Initializer> Build() override {
        ++build_count_;
        return std::move(source_);
    }

    int build_count_ = 0;
    std::unique_ptr<IFDv2Initializer> source_;
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
    std::unique_ptr<IFDv2Synchronizer> source_;
};

data_model::Selector MakeSelector(std::int64_t version, std::string state) {
    return data_model::Selector{
        data_model::Selector::State{version, std::move(state)}};
}

FDv2SourceResult MakeFullChangeSetResult(std::vector<ItemChange> items,
                                         data_model::Selector selector) {
    return FDv2SourceResult{FDv2SourceResult::ChangeSet{
        data_model::ChangeSet<ChangeSetData>{
            data_model::ChangeSetType::kFull,
            std::move(items),
            std::move(selector),
        },
        false,
    }};
}

}  // namespace

// ============================================================================
// Lifecycle
// ============================================================================

TEST(FDv2DataSystemTest, OfflineMode_NoFactories_StatusValid) {
    auto logger = MakeNullLogger();
    boost::asio::io_context ioc;
    data_components::DataSourceStatusManager status_manager;

    FDv2DataSystem ds({}, {}, ioc.get_executor(), &status_manager, logger);

    // Initialize with no sources; orchestration should not be posted.
    ds.Initialize();

    // Offline mode: status reaches Valid synchronously, store stays empty.
    EXPECT_EQ(status_manager.Status().State(),
              DataSourceStatus::DataSourceState::kValid);
    EXPECT_FALSE(ds.Initialized());
}

TEST(FDv2DataSystemTest, Destructor_TransitionsStatusToOff) {
    auto logger = MakeNullLogger();
    boost::asio::io_context ioc;
    data_components::DataSourceStatusManager status_manager;

    {
        FDv2DataSystem ds({}, {}, ioc.get_executor(), &status_manager, logger);
        ds.Initialize();
        ASSERT_EQ(status_manager.Status().State(),
                  DataSourceStatus::DataSourceState::kValid);
    }

    // After ~FDv2DataSystem, status is Off.
    EXPECT_EQ(status_manager.Status().State(),
              DataSourceStatus::DataSourceState::kOff);
}

// ============================================================================
// Initializer phase
// ============================================================================

TEST(FDv2DataSystemTest, InitializerWithBasis_AppliesAndStatusValid) {
    auto logger = MakeNullLogger();
    boost::asio::io_context ioc;
    data_components::DataSourceStatusManager status_manager;

    data_model::Flag flag_a;
    flag_a.key = "flagA";
    flag_a.version = 1;

    auto initializer =
        std::make_unique<MockInitializer>(MakeFullChangeSetResult(
            ChangeSetData{
                ItemChange{"flagA", data_model::FlagDescriptor(flag_a)},
            },
            MakeSelector(1, "state-1")));

    std::vector<std::unique_ptr<IFDv2InitializerFactory>> initializers;
    initializers.push_back(
        std::make_unique<OneShotInitializerFactory>(std::move(initializer)));

    FDv2DataSystem ds(std::move(initializers), {}, ioc.get_executor(),
                      &status_manager, logger);

    // Run the initializer to completion.
    ds.Initialize();
    ioc.run();

    // The Full changeset's flag is now visible and status is Valid.
    EXPECT_EQ(status_manager.Status().State(),
              DataSourceStatus::DataSourceState::kValid);
    EXPECT_TRUE(ds.Initialized());
    auto fetched = ds.GetFlag("flagA");
    ASSERT_TRUE(fetched);
    EXPECT_EQ(1u, fetched->version);
}

TEST(FDv2DataSystemTest, InitializerInterrupted_AdvancesToNextInitializer) {
    auto logger = MakeNullLogger();
    boost::asio::io_context ioc;
    data_components::DataSourceStatusManager status_manager;

    data_model::Flag flag_a;
    flag_a.key = "flagA";
    flag_a.version = 1;

    auto first = std::make_unique<MockInitializer>(
        FDv2SourceResult{FDv2SourceResult::Interrupted{
            FDv2SourceResult::ErrorInfo{
                FDv2SourceResult::ErrorInfo::ErrorKind::kNetworkError, 0,
                "boom", std::chrono::system_clock::now()},
            false,
        }});
    auto first_factory =
        std::make_unique<OneShotInitializerFactory>(std::move(first));
    auto* first_factory_ptr = first_factory.get();

    auto second = std::make_unique<MockInitializer>(MakeFullChangeSetResult(
        ChangeSetData{
            ItemChange{"flagA", data_model::FlagDescriptor(flag_a)},
        },
        MakeSelector(1, "state-1")));
    auto second_factory =
        std::make_unique<OneShotInitializerFactory>(std::move(second));
    auto* second_factory_ptr = second_factory.get();

    std::vector<std::unique_ptr<IFDv2InitializerFactory>> initializers;
    initializers.push_back(std::move(first_factory));
    initializers.push_back(std::move(second_factory));

    FDv2DataSystem ds(std::move(initializers), {}, ioc.get_executor(),
                      &status_manager, logger);

    // Run; first initializer fails, orchestrator should fall through to
    // the second.
    ds.Initialize();
    ioc.run();

    // Both factories were used; data from the second is in the store.
    EXPECT_EQ(1, first_factory_ptr->build_count_);
    EXPECT_EQ(1, second_factory_ptr->build_count_);
    EXPECT_TRUE(ds.Initialized());
    EXPECT_TRUE(ds.GetFlag("flagA"));
    EXPECT_EQ(status_manager.Status().State(),
              DataSourceStatus::DataSourceState::kValid);
}

TEST(FDv2DataSystemTest,
     InitializerChangeSet_WithoutSelector_ContinuesToNextInitializer) {
    auto logger = MakeNullLogger();
    boost::asio::io_context ioc;
    data_components::DataSourceStatusManager status_manager;

    data_model::Flag flag_a;
    flag_a.key = "flagA";
    flag_a.version = 1;

    data_model::Flag flag_b;
    flag_b.key = "flagB";
    flag_b.version = 1;

    // First initializer applies a Full changeset but with an EMPTY selector
    // (no basis). Orchestrator should still try the next initializer.
    auto first = std::make_unique<MockInitializer>(MakeFullChangeSetResult(
        ChangeSetData{
            ItemChange{"flagA", data_model::FlagDescriptor(flag_a)},
        },
        data_model::Selector{}));
    auto first_factory =
        std::make_unique<OneShotInitializerFactory>(std::move(first));
    auto* first_factory_ptr = first_factory.get();

    // Second initializer applies a Partial changeset (does not clear the
    // store) with a non-empty selector that ends the initializer phase.
    auto second = std::make_unique<MockInitializer>(
        FDv2SourceResult{FDv2SourceResult::ChangeSet{
            data_model::ChangeSet<ChangeSetData>{
                data_model::ChangeSetType::kPartial,
                ChangeSetData{
                    ItemChange{"flagB", data_model::FlagDescriptor(flag_b)},
                },
                MakeSelector(1, "state-1"),
            },
            false,
        }});
    auto second_factory =
        std::make_unique<OneShotInitializerFactory>(std::move(second));
    auto* second_factory_ptr = second_factory.get();

    std::vector<std::unique_ptr<IFDv2InitializerFactory>> initializers;
    initializers.push_back(std::move(first_factory));
    initializers.push_back(std::move(second_factory));

    FDv2DataSystem ds(std::move(initializers), {}, ioc.get_executor(),
                      &status_manager, logger);

    ds.Initialize();
    ioc.run();

    // Both initializers ran. Both flags are present: flagA was applied by the
    // first initializer (Full, no selector); flagB was applied by the second
    // (Partial, with selector) which doesn't clear the store.
    EXPECT_EQ(1, first_factory_ptr->build_count_);
    EXPECT_EQ(1, second_factory_ptr->build_count_);
    EXPECT_TRUE(ds.GetFlag("flagA"));
    EXPECT_TRUE(ds.GetFlag("flagB"));
}

TEST(FDv2DataSystemTest,
     InitializerWithBasis_StopsBeforeRemainingInitializers) {
    auto logger = MakeNullLogger();
    boost::asio::io_context ioc;
    data_components::DataSourceStatusManager status_manager;

    auto first = std::make_unique<MockInitializer>(
        MakeFullChangeSetResult(ChangeSetData{}, MakeSelector(1, "state-1")));
    auto first_factory =
        std::make_unique<OneShotInitializerFactory>(std::move(first));
    auto* first_factory_ptr = first_factory.get();

    // Second initializer should never be built.
    auto second = std::make_unique<MockInitializer>(
        FDv2SourceResult{FDv2SourceResult::Shutdown{}});
    auto second_factory =
        std::make_unique<OneShotInitializerFactory>(std::move(second));
    auto* second_factory_ptr = second_factory.get();

    std::vector<std::unique_ptr<IFDv2InitializerFactory>> initializers;
    initializers.push_back(std::move(first_factory));
    initializers.push_back(std::move(second_factory));

    FDv2DataSystem ds(std::move(initializers), {}, ioc.get_executor(),
                      &status_manager, logger);

    ds.Initialize();
    ioc.run();

    // First was built; second was skipped because the basis was already
    // received.
    EXPECT_EQ(1, first_factory_ptr->build_count_);
    EXPECT_EQ(0, second_factory_ptr->build_count_);
}

TEST(FDv2DataSystemTest, InitializerOnly_AllFail_TransitionsToOff) {
    auto logger = MakeNullLogger();
    boost::asio::io_context ioc;
    data_components::DataSourceStatusManager status_manager;

    auto init = std::make_unique<MockInitializer>(
        FDv2SourceResult{FDv2SourceResult::Interrupted{
            FDv2SourceResult::ErrorInfo{
                FDv2SourceResult::ErrorInfo::ErrorKind::kNetworkError, 0,
                "fail", std::chrono::system_clock::now()},
            false,
        }});

    std::vector<std::unique_ptr<IFDv2InitializerFactory>> initializers;
    initializers.push_back(
        std::make_unique<OneShotInitializerFactory>(std::move(init)));

    FDv2DataSystem ds(std::move(initializers), {}, ioc.get_executor(),
                      &status_manager, logger);

    // Run: initializer fails and there are no synchronizers to fall through to.
    ds.Initialize();
    ioc.run();

    // No data was ever applied; status transitions to Off.
    EXPECT_EQ(status_manager.Status().State(),
              DataSourceStatus::DataSourceState::kOff);
    EXPECT_FALSE(ds.Initialized());
}

// ============================================================================
// Synchronizer phase
// ============================================================================

TEST(FDv2DataSystemTest, SynchronizerChangeSet_AppliesAndStatusValid) {
    auto logger = MakeNullLogger();
    boost::asio::io_context ioc;
    data_components::DataSourceStatusManager status_manager;

    data_model::Flag flag_a;
    flag_a.key = "flagA";
    flag_a.version = 7;

    std::vector<FDv2SourceResult> results;
    results.push_back(MakeFullChangeSetResult(
        ChangeSetData{
            ItemChange{"flagA", data_model::FlagDescriptor(flag_a)},
        },
        MakeSelector(7, "v7")));
    auto sync = std::make_unique<MockSynchronizer>(std::move(results));

    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> synchronizers;
    synchronizers.push_back(
        std::make_unique<OneShotSynchronizerFactory>(std::move(sync)));

    FDv2DataSystem ds({}, std::move(synchronizers), ioc.get_executor(),
                      &status_manager, logger);

    // No initializers; orchestrator should hand directly to the synchronizer.
    ds.Initialize();
    ioc.run();

    EXPECT_EQ(status_manager.Status().State(),
              DataSourceStatus::DataSourceState::kValid);
    EXPECT_TRUE(ds.Initialized());
    auto fetched = ds.GetFlag("flagA");
    ASSERT_TRUE(fetched);
    EXPECT_EQ(7u, fetched->version);
}

TEST(FDv2DataSystemTest, SynchronizerGoodbye_StaysOnSameSynchronizer) {
    auto logger = MakeNullLogger();
    boost::asio::io_context ioc;
    data_components::DataSourceStatusManager status_manager;

    // Synchronizer first emits Goodbye, then exhausts (returns Shutdown).
    // The synchronizer is expected to handle the goodbye internally
    // (reconnecting); the orchestrator must NOT rotate.
    auto first =
        std::make_unique<MockSynchronizer>(std::vector<FDv2SourceResult>{
            FDv2SourceResult{FDv2SourceResult::Goodbye{std::nullopt, false}}});
    auto first_factory =
        std::make_unique<OneShotSynchronizerFactory>(std::move(first));
    auto* first_factory_ptr = first_factory.get();

    // Second factory should never be built; presence detects rotation.
    auto second =
        std::make_unique<MockSynchronizer>(std::vector<FDv2SourceResult>{});
    auto second_factory =
        std::make_unique<OneShotSynchronizerFactory>(std::move(second));
    auto* second_factory_ptr = second_factory.get();

    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> synchronizers;
    synchronizers.push_back(std::move(first_factory));
    synchronizers.push_back(std::move(second_factory));

    FDv2DataSystem ds({}, std::move(synchronizers), ioc.get_executor(),
                      &status_manager, logger);

    ds.Initialize();
    ioc.run();

    // Goodbye does not advance the factory cursor.
    EXPECT_EQ(1, first_factory_ptr->build_count_);
    EXPECT_EQ(0, second_factory_ptr->build_count_);
}

TEST(FDv2DataSystemTest, SynchronizerInterrupted_RetriesSameSynchronizer) {
    auto logger = MakeNullLogger();
    boost::asio::io_context ioc;
    data_components::DataSourceStatusManager status_manager;

    // Single synchronizer: first Next returns Interrupted, then (after the
    // orchestrator loops) a ChangeSet, then exhausts to Shutdown. There is
    // no second factory, so a rotation would be observable as "no second
    // build". Tests that Interrupted does NOT rotate.
    std::vector<FDv2SourceResult> results;
    results.push_back(FDv2SourceResult{FDv2SourceResult::Interrupted{
        FDv2SourceResult::ErrorInfo{
            FDv2SourceResult::ErrorInfo::ErrorKind::kNetworkError, 0,
            "transient", std::chrono::system_clock::now()},
        false,
    }});
    results.push_back(
        MakeFullChangeSetResult(ChangeSetData{}, MakeSelector(1, "state-1")));
    auto sync = std::make_unique<MockSynchronizer>(std::move(results));

    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> synchronizers;
    auto factory =
        std::make_unique<OneShotSynchronizerFactory>(std::move(sync));
    auto* factory_ptr = factory.get();
    synchronizers.push_back(std::move(factory));

    FDv2DataSystem ds({}, std::move(synchronizers), ioc.get_executor(),
                      &status_manager, logger);

    ds.Initialize();
    ioc.run();

    // Factory was built only once; the subsequent ChangeSet recovered the
    // status to Valid.
    EXPECT_EQ(1, factory_ptr->build_count_);
    EXPECT_EQ(status_manager.Status().State(),
              DataSourceStatus::DataSourceState::kValid);
}

TEST(FDv2DataSystemTest, SynchronizerNext_ReceivesUpdatedSelector) {
    auto logger = MakeNullLogger();
    boost::asio::io_context ioc;
    data_components::DataSourceStatusManager status_manager;

    // Initializer provides a basis with selector v1/state-1.
    auto initializer = std::make_unique<MockInitializer>(
        MakeFullChangeSetResult(ChangeSetData{}, MakeSelector(1, "state-1")));

    std::vector<std::unique_ptr<IFDv2InitializerFactory>> initializers;
    initializers.push_back(
        std::make_unique<OneShotInitializerFactory>(std::move(initializer)));

    // Synchronizer first returns a partial changeset with a NEW selector,
    // then exhausts (Shutdown) on the next call.
    std::vector<MockSynchronizer::NextCall> next_calls;
    std::vector<FDv2SourceResult> results;
    results.push_back(FDv2SourceResult{FDv2SourceResult::ChangeSet{
        data_model::ChangeSet<ChangeSetData>{
            data_model::ChangeSetType::kPartial,
            ChangeSetData{},
            MakeSelector(2, "state-2"),
        },
        false,
    }});
    auto sync = std::make_unique<MockSynchronizer>(std::move(results), nullptr,
                                                   &next_calls);

    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> synchronizers;
    synchronizers.push_back(
        std::make_unique<OneShotSynchronizerFactory>(std::move(sync)));

    FDv2DataSystem ds(std::move(initializers), std::move(synchronizers),
                      ioc.get_executor(), &status_manager, logger);

    ds.Initialize();
    ioc.run();

    // Two Next calls: first with the initializer's selector, second with the
    // selector updated by the partial changeset.
    ASSERT_EQ(2u, next_calls.size());
    ASSERT_TRUE(next_calls[0].second.value.has_value());
    EXPECT_EQ(1, next_calls[0].second.value->version);
    EXPECT_EQ("state-1", next_calls[0].second.value->state);
    ASSERT_TRUE(next_calls[1].second.value.has_value());
    EXPECT_EQ(2, next_calls[1].second.value->version);
    EXPECT_EQ("state-2", next_calls[1].second.value->state);
}

TEST(FDv2DataSystemTest,
     SynchronizerTerminalError_StatusInterruptedAndAdvance) {
    auto logger = MakeNullLogger();
    boost::asio::io_context ioc;
    data_components::DataSourceStatusManager status_manager;

    // First synchronizer applies a changeset (-> Valid), then fails terminally.
    std::vector<FDv2SourceResult> first_results;
    first_results.push_back(
        MakeFullChangeSetResult(ChangeSetData{}, MakeSelector(1, "state-1")));
    first_results.push_back(FDv2SourceResult{FDv2SourceResult::TerminalError{
        FDv2SourceResult::ErrorInfo{
            FDv2SourceResult::ErrorInfo::ErrorKind::kErrorResponse, 401,
            "unauthorized", std::chrono::system_clock::now()},
        false,
    }});
    auto first = std::make_unique<MockSynchronizer>(std::move(first_results));
    auto first_factory =
        std::make_unique<OneShotSynchronizerFactory>(std::move(first));
    auto* first_factory_ptr = first_factory.get();

    auto second = std::make_unique<MockSynchronizer>(
        std::vector<FDv2SourceResult>{});  // empty -> Shutdown
    auto second_factory =
        std::make_unique<OneShotSynchronizerFactory>(std::move(second));
    auto* second_factory_ptr = second_factory.get();

    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> synchronizers;
    synchronizers.push_back(std::move(first_factory));
    synchronizers.push_back(std::move(second_factory));

    FDv2DataSystem ds({}, std::move(synchronizers), ioc.get_executor(),
                      &status_manager, logger);

    ds.Initialize();
    ioc.run();

    // First synchronizer set status to Valid, then the terminal error pushed
    // it to Interrupted; after rotating to the second synchronizer (which
    // immediately exits via Shutdown), Interrupted is the final non-Off
    // state seen.
    EXPECT_EQ(1, first_factory_ptr->build_count_);
    EXPECT_EQ(1, second_factory_ptr->build_count_);
    EXPECT_EQ(status_manager.Status().State(),
              DataSourceStatus::DataSourceState::kInterrupted);
}

TEST(FDv2DataSystemTest, SynchronizerCycledExhaustion_TransitionsToOff) {
    auto logger = MakeNullLogger();
    boost::asio::io_context ioc;
    data_components::DataSourceStatusManager status_manager;

    // Single synchronizer that fails terminally on its first Next. The
    // orchestrator advances past the only factory and finds no more,
    // exhausting sources.
    auto sync =
        std::make_unique<MockSynchronizer>(std::vector<FDv2SourceResult>{
            FDv2SourceResult{FDv2SourceResult::TerminalError{
                FDv2SourceResult::ErrorInfo{
                    FDv2SourceResult::ErrorInfo::ErrorKind::kErrorResponse, 401,
                    "unauthorized", std::chrono::system_clock::now()},
                false,
            }}});

    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> synchronizers;
    synchronizers.push_back(
        std::make_unique<OneShotSynchronizerFactory>(std::move(sync)));

    FDv2DataSystem ds({}, std::move(synchronizers), ioc.get_executor(),
                      &status_manager, logger);

    // Synchronizer fails terminally; no more factories to try.
    ds.Initialize();
    ioc.run();

    // We cycled through all synchronizers; status transitions to Off.
    EXPECT_EQ(status_manager.Status().State(),
              DataSourceStatus::DataSourceState::kOff);
}
