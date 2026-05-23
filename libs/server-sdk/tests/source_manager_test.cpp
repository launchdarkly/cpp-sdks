#include <gtest/gtest.h>

#include <data_interfaces/source/ifdv2_synchronizer.hpp>
#include <data_interfaces/source/ifdv2_synchronizer_factory.hpp>
#include <data_systems/fdv2/source_manager.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace launchdarkly::server_side::data_interfaces;
using namespace launchdarkly::server_side::data_systems;

namespace {

// Stub synchronizer; SourceManager only cares that Build() returns one.
class StubSynchronizer : public IFDv2Synchronizer {
   public:
    launchdarkly::async::Future<FDv2SourceResult> Next(
        launchdarkly::data_model::Selector) override {
        return launchdarkly::async::MakeFuture(
            FDv2SourceResult{FDv2SourceResult::Shutdown{}});
    }

    void Close() override {}

    std::string const& Identity() const override {
        static std::string const id = "stub";
        return id;
    }
};

// Counts Build() calls for assertion. Tests don't run the returned
// synchronizer, so a fresh stub each time is fine.
class CountingFactory : public IFDv2SynchronizerFactory {
   public:
    std::unique_ptr<IFDv2Synchronizer> Build() override {
        ++build_count;
        return std::make_unique<StubSynchronizer>();
    }

    int build_count = 0;
};

class FDv1FallbackFactory : public CountingFactory {
   public:
    bool IsFDv1Fallback() const override { return true; }
};

}  // namespace

TEST(SourceManagerTest, EmptyManagerReportsZeroAvailable) {
    SourceManager mgr({});

    EXPECT_EQ(0u, mgr.AvailableSynchronizerCount());
    EXPECT_EQ(nullptr, mgr.NextSynchronizer());
    EXPECT_FALSE(mgr.IsPrimeSynchronizer());
    EXPECT_FALSE(mgr.IsCurrentSynchronizerFDv1Fallback());
}

TEST(SourceManagerTest, NextSynchronizerReturnsFirstThenWrapsAround) {
    auto f0 = std::make_unique<CountingFactory>();
    auto f1 = std::make_unique<CountingFactory>();
    auto* f0_ptr = f0.get();
    auto* f1_ptr = f1.get();
    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> factories;
    factories.push_back(std::move(f0));
    factories.push_back(std::move(f1));
    SourceManager mgr(std::move(factories));

    EXPECT_NE(nullptr, mgr.NextSynchronizer());
    EXPECT_TRUE(mgr.IsPrimeSynchronizer());
    EXPECT_NE(nullptr, mgr.NextSynchronizer());
    EXPECT_FALSE(mgr.IsPrimeSynchronizer());
    // Wraps back to the first factory.
    EXPECT_NE(nullptr, mgr.NextSynchronizer());
    EXPECT_TRUE(mgr.IsPrimeSynchronizer());

    EXPECT_EQ(2, f0_ptr->build_count);
    EXPECT_EQ(1, f1_ptr->build_count);
}

TEST(SourceManagerTest, BlockCurrentSynchronizerRemovesItFromRotation) {
    auto f0 = std::make_unique<CountingFactory>();
    auto f1 = std::make_unique<CountingFactory>();
    auto f2 = std::make_unique<CountingFactory>();
    auto* f0_ptr = f0.get();
    auto* f1_ptr = f1.get();
    auto* f2_ptr = f2.get();
    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> factories;
    factories.push_back(std::move(f0));
    factories.push_back(std::move(f1));
    factories.push_back(std::move(f2));
    SourceManager mgr(std::move(factories));

    // Advance to index 1 and block it.
    mgr.NextSynchronizer();  // 0
    mgr.NextSynchronizer();  // 1
    mgr.BlockCurrentSynchronizer();

    EXPECT_EQ(2u, mgr.AvailableSynchronizerCount());

    // From index 1, next should skip to 2.
    mgr.NextSynchronizer();
    EXPECT_FALSE(mgr.IsPrimeSynchronizer());

    // Then wrap to 0 (skipping blocked 1).
    mgr.NextSynchronizer();
    EXPECT_TRUE(mgr.IsPrimeSynchronizer());

    EXPECT_EQ(2, f0_ptr->build_count);
    EXPECT_EQ(1, f1_ptr->build_count);
    EXPECT_EQ(1, f2_ptr->build_count);
}

TEST(SourceManagerTest, AllBlockedReturnsNullAndZeroCount) {
    auto f0 = std::make_unique<CountingFactory>();
    auto f1 = std::make_unique<CountingFactory>();
    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> factories;
    factories.push_back(std::move(f0));
    factories.push_back(std::move(f1));
    SourceManager mgr(std::move(factories));

    mgr.NextSynchronizer();
    mgr.BlockCurrentSynchronizer();
    mgr.NextSynchronizer();
    mgr.BlockCurrentSynchronizer();

    EXPECT_EQ(0u, mgr.AvailableSynchronizerCount());
    EXPECT_EQ(nullptr, mgr.NextSynchronizer());
    EXPECT_FALSE(mgr.IsPrimeSynchronizer());
}

TEST(SourceManagerTest, ResetSourceIndexSendsNextCallToTheFirstAvailable) {
    auto f0 = std::make_unique<CountingFactory>();
    auto f1 = std::make_unique<CountingFactory>();
    auto f2 = std::make_unique<CountingFactory>();
    auto* f0_ptr = f0.get();
    auto* f2_ptr = f2.get();
    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> factories;
    factories.push_back(std::move(f0));
    factories.push_back(std::move(f1));
    factories.push_back(std::move(f2));
    SourceManager mgr(std::move(factories));

    // Walk to index 2.
    mgr.NextSynchronizer();
    mgr.NextSynchronizer();
    mgr.NextSynchronizer();
    EXPECT_EQ(1, f2_ptr->build_count);

    // Reset; next call should hit index 0 again.
    mgr.ResetSourceIndex();
    mgr.NextSynchronizer();
    EXPECT_TRUE(mgr.IsPrimeSynchronizer());
    EXPECT_EQ(2, f0_ptr->build_count);
}

TEST(SourceManagerTest, ResetSourceIndexSkipsBlockedFirstFactory) {
    auto f0 = std::make_unique<CountingFactory>();
    auto f1 = std::make_unique<CountingFactory>();
    auto* f0_ptr = f0.get();
    auto* f1_ptr = f1.get();
    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> factories;
    factories.push_back(std::move(f0));
    factories.push_back(std::move(f1));
    SourceManager mgr(std::move(factories));

    // Pick and block index 0.
    mgr.NextSynchronizer();
    mgr.BlockCurrentSynchronizer();

    // After reset, the next call should land on index 1 (first available),
    // and IsPrimeSynchronizer should treat index 1 as the prime.
    mgr.ResetSourceIndex();
    mgr.NextSynchronizer();
    EXPECT_TRUE(mgr.IsPrimeSynchronizer());

    EXPECT_EQ(1, f0_ptr->build_count);
    EXPECT_EQ(1, f1_ptr->build_count);
}

TEST(SourceManagerTest, IsCurrentSynchronizerFDv1FallbackFalseForFDv2Factory) {
    auto f0 = std::make_unique<CountingFactory>();
    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> factories;
    factories.push_back(std::move(f0));
    SourceManager mgr(std::move(factories));

    mgr.NextSynchronizer();
    EXPECT_FALSE(mgr.IsCurrentSynchronizerFDv1Fallback());
}

TEST(SourceManagerTest, FDv1FallbackFactoryStartsBlockedAndIsSkipped) {
    auto fdv2 = std::make_unique<CountingFactory>();
    auto fdv1 = std::make_unique<FDv1FallbackFactory>();
    auto* fdv1_ptr = fdv1.get();
    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> factories;
    factories.push_back(std::move(fdv2));
    factories.push_back(std::move(fdv1));
    SourceManager mgr(std::move(factories));

    EXPECT_EQ(1u, mgr.AvailableSynchronizerCount());
    mgr.NextSynchronizer();
    EXPECT_FALSE(mgr.IsCurrentSynchronizerFDv1Fallback());
    EXPECT_EQ(0, fdv1_ptr->build_count);
}

TEST(SourceManagerTest, SwitchToFDv1FallbackBlocksFDv2AndUnblocksFDv1) {
    auto fdv2 = std::make_unique<CountingFactory>();
    auto fdv1 = std::make_unique<FDv1FallbackFactory>();
    auto* fdv1_ptr = fdv1.get();
    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> factories;
    factories.push_back(std::move(fdv2));
    factories.push_back(std::move(fdv1));
    SourceManager mgr(std::move(factories));

    mgr.SwitchToFDv1Fallback();

    EXPECT_EQ(1u, mgr.AvailableSynchronizerCount());
    auto sync = mgr.NextSynchronizer();
    ASSERT_NE(sync, nullptr);
    EXPECT_EQ(1, fdv1_ptr->build_count);
    EXPECT_TRUE(mgr.IsCurrentSynchronizerFDv1Fallback());
}

TEST(SourceManagerTest, SwitchToFDv1FallbackWithoutAdapterBlocksEverything) {
    auto fdv2 = std::make_unique<CountingFactory>();
    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> factories;
    factories.push_back(std::move(fdv2));
    SourceManager mgr(std::move(factories));

    mgr.SwitchToFDv1Fallback();

    EXPECT_EQ(0u, mgr.AvailableSynchronizerCount());
    EXPECT_EQ(nullptr, mgr.NextSynchronizer());
}

TEST(SourceManagerTest, SwitchToFDv1FallbackUnblocksPreviouslyBlockedFDv2) {
    auto fdv2 = std::make_unique<CountingFactory>();
    auto fdv1 = std::make_unique<FDv1FallbackFactory>();
    auto* fdv1_ptr = fdv1.get();
    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> factories;
    factories.push_back(std::move(fdv2));
    factories.push_back(std::move(fdv1));
    SourceManager mgr(std::move(factories));

    mgr.NextSynchronizer();
    mgr.BlockCurrentSynchronizer();
    mgr.SwitchToFDv1Fallback();

    EXPECT_EQ(1u, mgr.AvailableSynchronizerCount());
    auto sync = mgr.NextSynchronizer();
    ASSERT_NE(sync, nullptr);
    EXPECT_EQ(1, fdv1_ptr->build_count);
    EXPECT_TRUE(mgr.IsCurrentSynchronizerFDv1Fallback());
}
