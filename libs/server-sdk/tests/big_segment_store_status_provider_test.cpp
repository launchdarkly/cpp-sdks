#include <gtest/gtest.h>

#include <data_components/big_segments/big_segment_store_status_provider.hpp>
#include <data_components/big_segments/big_segment_store_wrapper.hpp>

#include <launchdarkly/logging/null_logger.hpp>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

using launchdarkly::server_side::BigSegmentStoreStatus;
using launchdarkly::server_side::data_components::BigSegmentStoreStatusProvider;
using launchdarkly::server_side::data_components::BigSegmentStoreWrapper;
namespace integrations = launchdarkly::server_side::integrations;
namespace built = launchdarkly::server_side::config::built;
using namespace std::chrono_literals;

namespace {

// In-memory store whose metadata response the test controls.
class FakeBigSegmentStore : public integrations::IBigSegmentStore {
   public:
    GetMembershipResult GetMembership(
        std::string const&) const noexcept override {
        return integrations::Membership::FromSegmentRefs({}, {});
    }

    GetMetadataResult GetMetadata() const noexcept override {
        std::lock_guard lock(mutex_);
        return metadata_;
    }

    void SetMetadata(GetMetadataResult metadata) {
        std::lock_guard lock(mutex_);
        metadata_ = std::move(metadata);
    }

   private:
    mutable std::mutex mutex_;
    GetMetadataResult metadata_ =
        std::optional<integrations::StoreMetadata>{std::nullopt};
};

built::BigSegmentsConfig MakeConfig(
    std::shared_ptr<integrations::IBigSegmentStore> store,
    std::chrono::milliseconds poll_interval) {
    built::BigSegmentsConfig config;
    config.store = std::move(store);
    config.context_cache_size = 1000;
    config.context_cache_time = 5s;
    config.status_poll_interval = poll_interval;
    config.stale_after = 2min;
    return config;
}

}  // namespace

TEST(BigSegmentStoreStatusProviderTest, UnconfiguredReportsUnavailable) {
    BigSegmentStoreStatusProvider provider(nullptr);

    auto const status = provider.Status();
    EXPECT_FALSE(status.IsAvailable());
    EXPECT_FALSE(status.IsStale());

    // A listener can still be registered; it simply never fires.
    bool fired = false;
    auto connection = provider.OnBigSegmentStoreStatusChange(
        [&fired](BigSegmentStoreStatus) { fired = true; });
    ASSERT_NE(connection, nullptr);
    connection->Disconnect();
    EXPECT_FALSE(fired);
}

TEST(BigSegmentStoreStatusProviderTest, DelegatesStatusToWrapper) {
    auto store = std::make_shared<FakeBigSegmentStore>();
    store->SetMetadata(
        integrations::StoreMetadata{std::chrono::system_clock::now()});

    auto logger = launchdarkly::logging::NullLogger();
    boost::asio::io_context ioc;
    auto wrapper = std::make_shared<BigSegmentStoreWrapper>(
        MakeConfig(store, /*poll_interval=*/5s), ioc.get_executor(), logger);

    BigSegmentStoreStatusProvider provider(wrapper);

    // The wrapper polls inline on the first GetStatus, so fresh metadata is
    // reported as available and not stale.
    auto const status = provider.Status();
    EXPECT_TRUE(status.IsAvailable());
    EXPECT_FALSE(status.IsStale());
}

TEST(BigSegmentStoreStatusProviderTest, StaleMetadataReportedAsStale) {
    auto store = std::make_shared<FakeBigSegmentStore>();
    store->SetMetadata(
        integrations::StoreMetadata{std::chrono::system_clock::now() - 5min});

    auto logger = launchdarkly::logging::NullLogger();
    boost::asio::io_context ioc;
    auto wrapper = std::make_shared<BigSegmentStoreWrapper>(
        MakeConfig(store, /*poll_interval=*/5s), ioc.get_executor(), logger);

    BigSegmentStoreStatusProvider provider(wrapper);

    auto const status = provider.Status();
    EXPECT_TRUE(status.IsAvailable());
    EXPECT_TRUE(status.IsStale());
}

TEST(BigSegmentStoreStatusProviderTest, ListenerReceivesConvertedPublicStatus) {
    auto store = std::make_shared<FakeBigSegmentStore>();
    store->SetMetadata(
        integrations::StoreMetadata{std::chrono::system_clock::now()});

    auto logger = launchdarkly::logging::NullLogger();
    boost::asio::io_context ioc;
    auto work_guard = boost::asio::make_work_guard(ioc);
    std::thread io_thread([&ioc] { ioc.run(); });

    auto wrapper = std::make_shared<BigSegmentStoreWrapper>(
        MakeConfig(store, /*poll_interval=*/5ms), ioc.get_executor(), logger);

    BigSegmentStoreStatusProvider provider(wrapper);

    std::mutex mutex;
    std::condition_variable cv;
    std::optional<BigSegmentStoreStatus> last;
    auto connection = provider.OnBigSegmentStoreStatusChange(
        [&](BigSegmentStoreStatus status) {
            std::lock_guard lock(mutex);
            last = status;
            cv.notify_all();
        });

    wrapper->Start();

    // First poll broadcasts the initial healthy status through the adapter.
    {
        std::unique_lock lock(mutex);
        ASSERT_TRUE(cv.wait_for(
            lock, 1s, [&] { return last.has_value() && last->IsAvailable(); }));
        EXPECT_FALSE(last->IsStale());
    }

    // A store error flips availability, which the listener observes.
    store->SetMetadata(tl::make_unexpected("boom"));
    {
        std::unique_lock lock(mutex);
        ASSERT_TRUE(cv.wait_for(lock, 1s, [&] {
            return last.has_value() && !last->IsAvailable();
        }));
    }

    connection->Disconnect();

    work_guard.reset();
    ioc.stop();
    io_thread.join();
}
