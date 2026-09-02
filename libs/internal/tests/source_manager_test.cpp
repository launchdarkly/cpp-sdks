#include <gtest/gtest.h>

#include <launchdarkly/data_sources/fdv2/source_manager.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

// Stub synchronizer; SourceManager only cares that Build() returns one.
class StubSynchronizer {};

// Stands in for an SDK's synchronizer factory interface, which is all
// SourceManager requires of its type parameter.
class StubFactory {
   public:
    virtual std::unique_ptr<StubSynchronizer> Build() = 0;
    [[nodiscard]] virtual bool IsFDv1Fallback() const { return false; }
    virtual ~StubFactory() = default;
};

// Counts Build() calls for assertion. Tests don't run the returned
// synchronizer, so a fresh stub each time is fine.
class CountingFactory : public StubFactory {
   public:
    std::unique_ptr<StubSynchronizer> Build() override {
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

using SourceManager =
    launchdarkly::internal::data_sources::SourceManager<StubFactory>;

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
    std::vector<std::unique_ptr<StubFactory>> factories;
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
    std::vector<std::unique_ptr<StubFactory>> factories;
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
    std::vector<std::unique_ptr<StubFactory>> factories;
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
    std::vector<std::unique_ptr<StubFactory>> factories;
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
    std::vector<std::unique_ptr<StubFactory>> factories;
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
    std::vector<std::unique_ptr<StubFactory>> factories;
    factories.push_back(std::move(f0));
    SourceManager mgr(std::move(factories));

    mgr.NextSynchronizer();
    EXPECT_FALSE(mgr.IsCurrentSynchronizerFDv1Fallback());
}

TEST(SourceManagerTest, FDv1FallbackFactoryStartsBlockedAndIsSkipped) {
    auto fdv2 = std::make_unique<CountingFactory>();
    auto fdv1 = std::make_unique<FDv1FallbackFactory>();
    auto* fdv1_ptr = fdv1.get();
    std::vector<std::unique_ptr<StubFactory>> factories;
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
    std::vector<std::unique_ptr<StubFactory>> factories;
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
    std::vector<std::unique_ptr<StubFactory>> factories;
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
    std::vector<std::unique_ptr<StubFactory>> factories;
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

TEST(SourceManagerTest, SwitchBackToFDv2UnblocksFDv2AndBlocksFDv1) {
    auto fdv2 = std::make_unique<CountingFactory>();
    auto* fdv2_ptr = fdv2.get();
    auto fdv1 = std::make_unique<FDv1FallbackFactory>();
    std::vector<std::unique_ptr<StubFactory>> factories;
    factories.push_back(std::move(fdv2));
    factories.push_back(std::move(fdv1));
    SourceManager mgr(std::move(factories));

    // Switch to FDv1 first, then back to FDv2.
    mgr.SwitchToFDv1Fallback();
    mgr.SwitchBackToFDv2();

    EXPECT_EQ(1u, mgr.AvailableSynchronizerCount());
    auto sync = mgr.NextSynchronizer();
    ASSERT_NE(sync, nullptr);
    EXPECT_EQ(1, fdv2_ptr->build_count);
    EXPECT_FALSE(mgr.IsCurrentSynchronizerFDv1Fallback());
}

TEST(SourceManagerTest, SwitchBackToFDv2UnblocksTerminallyFailedFDv2Factory) {
    auto fdv2 = std::make_unique<CountingFactory>();
    auto* fdv2_ptr = fdv2.get();
    std::vector<std::unique_ptr<StubFactory>> factories;
    factories.push_back(std::move(fdv2));
    SourceManager mgr(std::move(factories));

    // Simulate a terminal error blocking the FDv2 factory.
    mgr.NextSynchronizer();
    mgr.BlockCurrentSynchronizer();
    EXPECT_EQ(0u, mgr.AvailableSynchronizerCount());

    mgr.SwitchBackToFDv2();

    // Previously-blocked FDv2 factory is now available again.
    EXPECT_EQ(1u, mgr.AvailableSynchronizerCount());
    auto sync = mgr.NextSynchronizer();
    ASSERT_NE(sync, nullptr);
    EXPECT_EQ(2, fdv2_ptr->build_count);
}
