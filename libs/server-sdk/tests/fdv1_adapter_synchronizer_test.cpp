#include <gtest/gtest.h>

#include <data_systems/fdv2/fdv1_adapter_synchronizer.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <utility>

using namespace launchdarkly;
using namespace launchdarkly::server_side;
using namespace launchdarkly::server_side::data_components;
using namespace launchdarkly::server_side::data_interfaces;
using namespace launchdarkly::server_side::data_systems;
using namespace std::chrono_literals;

namespace {

// Mock FDv1 source: records StartAsync and ShutdownAsync calls and exposes
// the IDestination it was given so the test can drive Init/Upsert.
class MockFDv1Source final : public IDataSynchronizer {
   public:
    explicit MockFDv1Source(DataSourceStatusManager& /*status_manager*/) {}

    void StartAsync(IDestination* destination,
                    data_model::SDKDataSet const* bootstrap) override {
        ++start_count;
        destination_ = destination;
        bootstrap_was_null = (bootstrap == nullptr);
    }

    void ShutdownAsync(std::function<void()> completion) override {
        ++shutdown_count;
        if (completion) {
            completion();
        }
    }

    std::string const& Identity() const override {
        static std::string const id = "mock fdv1";
        return id;
    }

    IDestination* destination_ = nullptr;
    int start_count = 0;
    int shutdown_count = 0;
    bool bootstrap_was_null = false;
};

// Returns a SourceBuilder closure that constructs a MockFDv1Source. The
// resulting source and the adapter's internal status manager are exposed
// through the out parameters; pass nullptr for either to skip.
FDv1AdapterSynchronizer::SourceBuilder MakeMockBuilder(
    MockFDv1Source** out_source = nullptr,
    DataSourceStatusManager** out_sm = nullptr) {
    return [out_source, out_sm](DataSourceStatusManager& sm) {
        if (out_sm) {
            *out_sm = &sm;
        }
        auto source = std::make_shared<MockFDv1Source>(sm);
        if (out_source) {
            *out_source = source.get();
        }
        return source;
    };
}

// FDv1 source that mirrors the real StreamingDataSource/PollingDataSource
// teardown. It keeps itself alive with shared_from_this, defers its
// ShutdownAsync completion, and delivers one more callback to the destination
// before signaling completion. The contract requires the destination to stay
// valid until then.
class DeferredCompletionFDv1Source final
    : public IDataSynchronizer,
      public std::enable_shared_from_this<DeferredCompletionFDv1Source> {
   public:
    explicit DeferredCompletionFDv1Source(boost::asio::any_io_executor executor)
        : executor_(std::move(executor)) {}

    void StartAsync(IDestination* destination,
                    data_model::SDKDataSet const* /*bootstrap*/) override {
        destination_ = destination;
    }

    // Posts the drain: one in-flight upsert into the destination, then the
    // completion. Holds shared_from_this so the source outlives the adapter's
    // teardown, as the real sources do during async shutdown.
    void ShutdownAsync(std::function<void()> completion) override {
        boost::asio::post(executor_, [self = shared_from_this(),
                                      completion = std::move(completion)]() {
            data_model::Flag flag;
            flag.key = "late";
            flag.version = 1;
            self->destination_->Upsert("late",
                                       data_model::FlagDescriptor(flag));
            self->completion_invoked = true;
            if (completion) {
                completion();
            }
        });
    }

    std::string const& Identity() const override {
        static std::string const id = "deferred completion fdv1";
        return id;
    }

    boost::asio::any_io_executor executor_;
    IDestination* destination_ = nullptr;
    bool completion_invoked = false;
};

}  // namespace

// The IDataSynchronizer contract states the destination pointer stays valid
// until the ShutdownAsync completion handler is called. This source upserts
// once during shutdown and then fires the completion. The adapter must keep the
// destination alive until the completion fires. Otherwise the drain lands on
// freed memory, a use-after-free.
TEST(FDv1AdapterSynchronizerTest, DestinationStaysValidUntilShutdownCompletes) {
    boost::asio::io_context ioc;
    auto source =
        std::make_shared<DeferredCompletionFDv1Source>(ioc.get_executor());
    {
        FDv1AdapterSynchronizer adapter(
            [source](DataSourceStatusManager&) { return source; });
        adapter.Next(data_model::Selector{});  // triggers StartAsync
    }  // adapter destroyed: Close() requests shutdown, the drain is still
       // pending

    // StartAsync handed the destination to the source.
    ASSERT_NE(source->destination_, nullptr);

    // Run the pending drain: the source upserts into the destination, then
    // fires the shutdown completion.
    ioc.run();

    EXPECT_TRUE(source->completion_invoked);
}

TEST(FDv1AdapterSynchronizerTest, FirstNextStartsFDv1Source) {
    MockFDv1Source* source = nullptr;
    FDv1AdapterSynchronizer adapter(MakeMockBuilder(&source));

    auto future = adapter.Next(data_model::Selector{});

    EXPECT_EQ(1, source->start_count);
    EXPECT_TRUE(source->bootstrap_was_null);
    EXPECT_FALSE(future.IsFinished());
}

TEST(FDv1AdapterSynchronizerTest, SecondNextDoesNotRestartSource) {
    MockFDv1Source* source = nullptr;
    FDv1AdapterSynchronizer adapter(MakeMockBuilder(&source));

    auto first = adapter.Next(data_model::Selector{});
    source->destination_->Init(data_model::SDKDataSet{});
    auto result = first.WaitForResult(1s);
    ASSERT_TRUE(result.has_value());
    adapter.Next(data_model::Selector{});

    EXPECT_EQ(1, source->start_count);
}

TEST(FDv1AdapterSynchronizerTest, FDv1InitProducesFullChangeSet) {
    MockFDv1Source* source = nullptr;
    FDv1AdapterSynchronizer adapter(MakeMockBuilder(&source));

    auto future = adapter.Next(data_model::Selector{});

    data_model::SDKDataSet data_set;
    data_model::Flag flag;
    flag.key = "flagA";
    flag.version = 1;
    data_set.flags.emplace("flagA", data_model::FlagDescriptor(flag));
    source->destination_->Init(std::move(data_set));

    auto result = future.WaitForResult(1s);
    ASSERT_TRUE(result.has_value());
    auto* change_set = std::get_if<FDv2SourceResult::ChangeSet>(&result->value);
    ASSERT_NE(change_set, nullptr);
    EXPECT_EQ(data_model::ChangeSetType::kFull, change_set->change_set.type);
    ASSERT_EQ(1u, change_set->change_set.data.size());
    EXPECT_EQ("flagA", change_set->change_set.data[0].key);
    EXPECT_FALSE(change_set->change_set.selector.value.has_value());
    EXPECT_FALSE(result->fdv1_fallback);
}

TEST(FDv1AdapterSynchronizerTest, FDv1UpsertProducesPartialChangeSet) {
    MockFDv1Source* source = nullptr;
    FDv1AdapterSynchronizer adapter(MakeMockBuilder(&source));

    // Flag upsert.
    auto flag_future = adapter.Next(data_model::Selector{});
    data_model::Flag flag;
    flag.key = "flagA";
    flag.version = 2;
    source->destination_->Upsert("flagA", data_model::FlagDescriptor(flag));

    auto flag_result = flag_future.WaitForResult(1s);
    ASSERT_TRUE(flag_result.has_value());
    auto* flag_change_set =
        std::get_if<FDv2SourceResult::ChangeSet>(&flag_result->value);
    ASSERT_NE(flag_change_set, nullptr);
    EXPECT_EQ(data_model::ChangeSetType::kPartial,
              flag_change_set->change_set.type);
    ASSERT_EQ(1u, flag_change_set->change_set.data.size());
    EXPECT_EQ("flagA", flag_change_set->change_set.data[0].key);

    // Segment upsert.
    auto seg_future = adapter.Next(data_model::Selector{});
    data_model::Segment seg;
    seg.key = "segA";
    seg.version = 3;
    source->destination_->Upsert("segA", data_model::SegmentDescriptor(seg));

    auto seg_result = seg_future.WaitForResult(1s);
    ASSERT_TRUE(seg_result.has_value());
    auto* seg_change_set =
        std::get_if<FDv2SourceResult::ChangeSet>(&seg_result->value);
    ASSERT_NE(seg_change_set, nullptr);
    EXPECT_EQ(data_model::ChangeSetType::kPartial,
              seg_change_set->change_set.type);
    ASSERT_EQ(1u, seg_change_set->change_set.data.size());
    EXPECT_EQ("segA", seg_change_set->change_set.data[0].key);
}

TEST(FDv1AdapterSynchronizerTest, ClosePendingNextReturnsShutdown) {
    FDv1AdapterSynchronizer adapter(MakeMockBuilder());

    auto future = adapter.Next(data_model::Selector{});
    EXPECT_FALSE(future.IsFinished());

    adapter.Close();

    auto result = future.WaitForResult(1s);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Shutdown>(result->value));
}

TEST(FDv1AdapterSynchronizerTest, CloseShutsDownStartedFDv1Source) {
    MockFDv1Source* source = nullptr;
    FDv1AdapterSynchronizer adapter(MakeMockBuilder(&source));

    adapter.Next(data_model::Selector{});
    adapter.Close();

    EXPECT_EQ(1, source->shutdown_count);
}

TEST(FDv1AdapterSynchronizerTest, CloseWithoutStartDoesNotShutDownFDv1Source) {
    MockFDv1Source* source = nullptr;
    FDv1AdapterSynchronizer adapter(MakeMockBuilder(&source));

    // No Next() call — FDv1 source was never started.
    adapter.Close();

    EXPECT_EQ(0, source->start_count);
    EXPECT_EQ(0, source->shutdown_count);
}

TEST(FDv1AdapterSynchronizerTest, QueuedResultsDrainInFifoOrder) {
    MockFDv1Source* source = nullptr;
    FDv1AdapterSynchronizer adapter(MakeMockBuilder(&source));

    // Start the source by satisfying one Next() with an Init.
    auto first = adapter.Next(data_model::Selector{});
    source->destination_->Init(data_model::SDKDataSet{});
    first.WaitForResult(1s);

    // Two upserts queue with no Next() in flight.
    data_model::Flag flag_a;
    flag_a.key = "a";
    data_model::Flag flag_b;
    flag_b.key = "b";
    source->destination_->Upsert("a", data_model::FlagDescriptor(flag_a));
    source->destination_->Upsert("b", data_model::FlagDescriptor(flag_b));

    // Drain in FIFO order.
    auto r1 = adapter.Next(data_model::Selector{}).WaitForResult(1s);
    auto r2 = adapter.Next(data_model::Selector{}).WaitForResult(1s);
    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());
    auto* cs1 = std::get_if<FDv2SourceResult::ChangeSet>(&r1->value);
    auto* cs2 = std::get_if<FDv2SourceResult::ChangeSet>(&r2->value);
    ASSERT_NE(cs1, nullptr);
    ASSERT_NE(cs2, nullptr);
    ASSERT_EQ(1u, cs1->change_set.data.size());
    ASSERT_EQ(1u, cs2->change_set.data.size());
    EXPECT_EQ("a", cs1->change_set.data[0].key);
    EXPECT_EQ("b", cs2->change_set.data[0].key);
}

TEST(FDv1AdapterSynchronizerTest, InterruptedStatusProducesInterruptedResult) {
    DataSourceStatusManager* source_manager = nullptr;
    FDv1AdapterSynchronizer adapter(MakeMockBuilder(nullptr, &source_manager));

    // kInterrupted from kInitializing stays kInitializing; drive past first.
    source_manager->SetState(DataSourceStatus::DataSourceState::kValid);

    auto future = adapter.Next(data_model::Selector{});
    source_manager->SetState(
        DataSourceStatus::DataSourceState::kInterrupted,
        DataSourceStatus::ErrorInfo::ErrorKind::kNetworkError, "boom");

    auto result = future.WaitForResult(1s);
    ASSERT_TRUE(result.has_value());
    auto* interrupted =
        std::get_if<FDv2SourceResult::Interrupted>(&result->value);
    ASSERT_NE(interrupted, nullptr);
    EXPECT_EQ(DataSourceStatus::ErrorInfo::ErrorKind::kNetworkError,
              interrupted->error.Kind());
}

TEST(FDv1AdapterSynchronizerTest,
     InitializingWithErrorProducesInterruptedResult) {
    DataSourceStatusManager* source_manager = nullptr;
    FDv1AdapterSynchronizer adapter(MakeMockBuilder(nullptr, &source_manager));

    auto future = adapter.Next(data_model::Selector{});
    source_manager->SetState(
        DataSourceStatus::DataSourceState::kInterrupted,
        DataSourceStatus::ErrorInfo::ErrorKind::kNetworkError, "boom");

    auto result = future.WaitForResult(1s);
    ASSERT_TRUE(result.has_value());
    auto* interrupted =
        std::get_if<FDv2SourceResult::Interrupted>(&result->value);
    ASSERT_NE(interrupted, nullptr);
    EXPECT_EQ(DataSourceStatus::ErrorInfo::ErrorKind::kNetworkError,
              interrupted->error.Kind());
}

TEST(FDv1AdapterSynchronizerTest, OffStatusProducesTerminalErrorResult) {
    DataSourceStatusManager* source_manager = nullptr;
    FDv1AdapterSynchronizer adapter(MakeMockBuilder(nullptr, &source_manager));

    auto future = adapter.Next(data_model::Selector{});
    source_manager->SetState(
        DataSourceStatus::DataSourceState::kOff,
        DataSourceStatus::ErrorInfo::ErrorKind::kErrorResponse, "401");

    auto result = future.WaitForResult(1s);
    ASSERT_TRUE(result.has_value());
    auto* terminal =
        std::get_if<FDv2SourceResult::TerminalError>(&result->value);
    ASSERT_NE(terminal, nullptr);
    EXPECT_EQ(DataSourceStatus::ErrorInfo::ErrorKind::kErrorResponse,
              terminal->error.Kind());
}

TEST(FDv1AdapterSynchronizerTest, NextAfterCloseReturnsShutdown) {
    FDv1AdapterSynchronizer adapter(MakeMockBuilder());

    adapter.Close();
    auto future = adapter.Next(data_model::Selector{});

    auto result = future.WaitForResult(1s);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Shutdown>(result->value));
}
