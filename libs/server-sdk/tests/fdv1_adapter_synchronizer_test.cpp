#include <gtest/gtest.h>

#include <data_systems/fdv2/fdv1_adapter_synchronizer.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <utility>

using namespace launchdarkly;
using namespace launchdarkly::server_side::data_interfaces;
using namespace launchdarkly::server_side::data_systems;
using namespace std::chrono_literals;

namespace {

// Mock FDv1 source: records StartAsync and ShutdownAsync calls and exposes
// the IDestination it was given so the test can drive Init/Upsert.
class MockFDv1Source final : public IDataSynchronizer {
   public:
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

}  // namespace

TEST(FDv1AdapterSynchronizerTest, FirstNextStartsFDv1Source) {
    auto source = std::make_unique<MockFDv1Source>();
    auto* source_ptr = source.get();
    FDv1AdapterSynchronizer adapter(std::move(source));

    auto future = adapter.Next(data_model::Selector{});

    EXPECT_EQ(1, source_ptr->start_count);
    EXPECT_TRUE(source_ptr->bootstrap_was_null);
    EXPECT_FALSE(future.IsFinished());
}

TEST(FDv1AdapterSynchronizerTest, SecondNextDoesNotRestartSource) {
    auto source = std::make_unique<MockFDv1Source>();
    auto* source_ptr = source.get();
    FDv1AdapterSynchronizer adapter(std::move(source));

    auto first = adapter.Next(data_model::Selector{});
    source_ptr->destination_->Init(data_model::SDKDataSet{});
    auto result = first.WaitForResult(1s);
    ASSERT_TRUE(result.has_value());
    adapter.Next(data_model::Selector{});

    EXPECT_EQ(1, source_ptr->start_count);
}

TEST(FDv1AdapterSynchronizerTest, FDv1InitProducesFullChangeSet) {
    auto source = std::make_unique<MockFDv1Source>();
    auto* source_ptr = source.get();
    FDv1AdapterSynchronizer adapter(std::move(source));

    auto future = adapter.Next(data_model::Selector{});

    data_model::SDKDataSet data_set;
    data_model::Flag flag;
    flag.key = "flagA";
    flag.version = 1;
    data_set.flags.emplace("flagA", data_model::FlagDescriptor(flag));
    source_ptr->destination_->Init(std::move(data_set));

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
    auto source = std::make_unique<MockFDv1Source>();
    auto* source_ptr = source.get();
    FDv1AdapterSynchronizer adapter(std::move(source));

    // Flag upsert.
    auto flag_future = adapter.Next(data_model::Selector{});
    data_model::Flag flag;
    flag.key = "flagA";
    flag.version = 2;
    source_ptr->destination_->Upsert("flagA", data_model::FlagDescriptor(flag));

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
    source_ptr->destination_->Upsert("segA",
                                     data_model::SegmentDescriptor(seg));

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
    auto source = std::make_unique<MockFDv1Source>();
    FDv1AdapterSynchronizer adapter(std::move(source));

    auto future = adapter.Next(data_model::Selector{});
    EXPECT_FALSE(future.IsFinished());

    adapter.Close();

    auto result = future.WaitForResult(1s);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Shutdown>(result->value));
}

TEST(FDv1AdapterSynchronizerTest, CloseShutsDownStartedFDv1Source) {
    auto source = std::make_unique<MockFDv1Source>();
    auto* source_ptr = source.get();
    FDv1AdapterSynchronizer adapter(std::move(source));

    adapter.Next(data_model::Selector{});
    adapter.Close();

    EXPECT_EQ(1, source_ptr->shutdown_count);
}

TEST(FDv1AdapterSynchronizerTest, CloseWithoutStartDoesNotShutDownFDv1Source) {
    auto source = std::make_unique<MockFDv1Source>();
    auto* source_ptr = source.get();
    FDv1AdapterSynchronizer adapter(std::move(source));

    // No Next() call — FDv1 source was never started.
    adapter.Close();

    EXPECT_EQ(0, source_ptr->start_count);
    EXPECT_EQ(0, source_ptr->shutdown_count);
}

TEST(FDv1AdapterSynchronizerTest, NextAfterCloseReturnsShutdown) {
    auto source = std::make_unique<MockFDv1Source>();
    FDv1AdapterSynchronizer adapter(std::move(source));

    adapter.Close();
    auto future = adapter.Next(data_model::Selector{});

    auto result = future.WaitForResult(1s);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(
        std::holds_alternative<FDv2SourceResult::Shutdown>(result->value));
}
