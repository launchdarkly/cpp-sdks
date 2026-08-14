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
    GetMetadataResult metadata_;
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

TEST(BigSegmentStoreStatusProviderTest, StatusAndListenerReflectStoreTransitions) {
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

    // Fresh metadata is reported as available and not stale.
    {
        std::unique_lock lock(mutex);
        ASSERT_TRUE(cv.wait_for(lock, 1s, [&] {
            return last.has_value() && last->IsAvailable() && !last->IsStale();
        }));
        EXPECT_EQ(provider.Status(), *last);
    }

    // Old metadata is reported as available but stale.
    store->SetMetadata(
        integrations::StoreMetadata{std::chrono::system_clock::now() - 5min});
    {
        std::unique_lock lock(mutex);
        ASSERT_TRUE(cv.wait_for(
            lock, 1s, [&] { return last.has_value() && last->IsStale(); }));
        EXPECT_TRUE(last->IsAvailable());
        EXPECT_EQ(provider.Status(), *last);
    }

    // A store error flips availability.
    store->SetMetadata(tl::make_unexpected("boom"));
    {
        std::unique_lock lock(mutex);
        ASSERT_TRUE(cv.wait_for(lock, 1s, [&] {
            return last.has_value() && !last->IsAvailable();
        }));
        EXPECT_EQ(provider.Status(), *last);
    }

    connection->Disconnect();

    work_guard.reset();
    ioc.stop();
    io_thread.join();
}
