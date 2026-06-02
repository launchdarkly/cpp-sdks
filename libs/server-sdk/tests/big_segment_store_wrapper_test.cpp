#include <gtest/gtest.h>

#include <data_components/big_segments/big_segment_store_wrapper.hpp>

#include <launchdarkly/logging/null_logger.hpp>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

using launchdarkly::server_side::data_components::BigSegmentsStatus;
using launchdarkly::server_side::data_components::BigSegmentStoreStatus;
using launchdarkly::server_side::data_components::BigSegmentStoreWrapper;
namespace integrations = launchdarkly::server_side::integrations;
namespace built = launchdarkly::server_side::config::built;
using namespace std::chrono_literals;

namespace {

// In-memory store whose metadata response the test controls. Records how many
// times GetMetadata was called and lets a test block until a target count.
class FakeBigSegmentStore : public integrations::IBigSegmentStore {
   public:
    GetMembershipResult GetMembership(std::string const&) const override {
        std::unique_lock lock(mutex_);
        ++membership_calls_;
        cv_.notify_all();
        // If gated, hold the query "in flight" until ReleaseMembership opens
        // it.
        cv_.wait(lock, [this] { return gate_open_; });
        return membership_;
    }

    GetMetadataResult GetMetadata() const override {
        std::lock_guard lock(mutex_);
        ++metadata_calls_;
        cv_.notify_all();
        return metadata_;
    }

    void SetMembership(GetMembershipResult membership) {
        std::lock_guard lock(mutex_);
        membership_ = std::move(membership);
    }

    void SetMetadata(GetMetadataResult metadata) {
        std::lock_guard lock(mutex_);
        metadata_ = std::move(metadata);
    }

    int MembershipCalls() const {
        std::lock_guard lock(mutex_);
        return membership_calls_;
    }

    int MetadataCalls() const {
        std::lock_guard lock(mutex_);
        return metadata_calls_;
    }

    void WaitForMembershipCalls(int n, std::chrono::milliseconds timeout) {
        std::unique_lock lock(mutex_);
        cv_.wait_for(lock, timeout, [&] { return membership_calls_ >= n; });
    }

    void WaitForMetadataCalls(int n, std::chrono::milliseconds timeout) {
        std::unique_lock lock(mutex_);
        cv_.wait_for(lock, timeout, [&] { return metadata_calls_ >= n; });
    }

    void BlockMembership() {
        std::lock_guard lock(mutex_);
        gate_open_ = false;
    }

    void ReleaseMembership() {
        {
            std::lock_guard lock(mutex_);
            gate_open_ = true;
        }
        cv_.notify_all();
    }

   private:
    mutable std::mutex mutex_;
    // Notified whenever a call is recorded or the gate opens. Tests wait on it
    // for call counts; a gated GetMembership waits on it for the gate to open.
    mutable std::condition_variable cv_;

    // All of the following are protected by mutex_.
    mutable int metadata_calls_ = 0;
    mutable int membership_calls_ = 0;
    GetMetadataResult metadata_ =
        std::optional<integrations::StoreMetadata>{std::nullopt};
    GetMembershipResult membership_ =
        integrations::Membership::FromSegmentRefs({}, {});
    // When false, GetMembership blocks until ReleaseMembership().
    bool gate_open_ = true;
};

// Collects status broadcasts and lets a test block until a target count.
class StatusCollector {
   public:
    void Add(BigSegmentStoreStatus status) {
        std::lock_guard lock(mutex_);
        statuses_.push_back(status);
        cv_.notify_all();
    }

    bool WaitForCount(std::size_t n, std::chrono::milliseconds timeout) {
        std::unique_lock lock(mutex_);
        return cv_.wait_for(lock, timeout,
                            [&] { return statuses_.size() >= n; });
    }

    std::vector<BigSegmentStoreStatus> Statuses() const {
        std::lock_guard lock(mutex_);
        return statuses_;
    }

   private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<BigSegmentStoreStatus> statuses_;
};

built::BigSegmentsConfig MakeConfig(
    std::shared_ptr<integrations::IBigSegmentStore> store,
    std::chrono::milliseconds poll_interval,
    std::chrono::milliseconds stale_after) {
    built::BigSegmentsConfig config;
    config.store = std::move(store);
    config.context_cache_size = 1000;
    config.context_cache_time = 5s;
    config.status_poll_interval = poll_interval;
    config.stale_after = stale_after;
    return config;
}

// Builds a wrapper over a store with the given metadata and returns the status
// from its initial synchronous poll. The executor is never run, since GetStatus
// polls inline when no background poll has happened yet.
BigSegmentStoreStatus StatusForMetadata(
    integrations::IBigSegmentStore::GetMetadataResult metadata) {
    auto store = std::make_shared<FakeBigSegmentStore>();
    store->SetMetadata(std::move(metadata));

    auto logger = launchdarkly::logging::NullLogger();
    boost::asio::io_context ioc;
    auto wrapper = std::make_shared<BigSegmentStoreWrapper>(
        MakeConfig(store, /*poll_interval=*/5s, /*stale_after=*/2min),
        ioc.get_executor(), logger);
    return wrapper->GetStatus();
}

}  // namespace

TEST(BigSegmentStoreWrapperStatusTest, FreshMetadataIsHealthy) {
    auto status = StatusForMetadata(
        integrations::StoreMetadata{std::chrono::system_clock::now()});
    EXPECT_TRUE(status.available);
    EXPECT_FALSE(status.stale);
}

TEST(BigSegmentStoreWrapperStatusTest, OldMetadataIsStale) {
    auto status = StatusForMetadata(
        integrations::StoreMetadata{std::chrono::system_clock::now() - 5min});
    EXPECT_TRUE(status.available);
    EXPECT_TRUE(status.stale);
}

TEST(BigSegmentStoreWrapperStatusTest, NeverSyncedIsStale) {
    auto status = StatusForMetadata(
        std::optional<integrations::StoreMetadata>{std::nullopt});
    EXPECT_TRUE(status.available);
    EXPECT_TRUE(status.stale);
}

TEST(BigSegmentStoreWrapperStatusTest, StoreErrorIsUnavailable) {
    auto status = StatusForMetadata(tl::make_unexpected("boom"));
    EXPECT_FALSE(status.available);
    EXPECT_FALSE(status.stale);
}

TEST(BigSegmentStoreWrapperStatusTest, GetStatusCachesFirstPollResult) {
    auto store = std::make_shared<FakeBigSegmentStore>();
    store->SetMetadata(
        integrations::StoreMetadata{std::chrono::system_clock::now()});

    auto logger = launchdarkly::logging::NullLogger();
    boost::asio::io_context ioc;
    auto wrapper = std::make_shared<BigSegmentStoreWrapper>(
        MakeConfig(store, /*poll_interval=*/5s, /*stale_after=*/2min),
        ioc.get_executor(), logger);

    auto first = wrapper->GetStatus();
    auto second = wrapper->GetStatus();
    EXPECT_EQ(first, second);

    // Only the first GetStatus queries the store; the rest reuse its result.
    EXPECT_EQ(1, store->MetadataCalls());
}

TEST(BigSegmentStoreWrapperStatusTest, BackgroundPollNotifiesOnlyOnChange) {
    auto store = std::make_shared<FakeBigSegmentStore>();
    store->SetMetadata(
        integrations::StoreMetadata{std::chrono::system_clock::now()});

    auto logger = launchdarkly::logging::NullLogger();
    boost::asio::io_context ioc;
    auto work_guard = boost::asio::make_work_guard(ioc);
    std::thread io_thread([&ioc] { ioc.run(); });

    auto wrapper = std::make_shared<BigSegmentStoreWrapper>(
        MakeConfig(store, /*poll_interval=*/5ms, /*stale_after=*/2min),
        ioc.get_executor(), logger);

    StatusCollector collector;
    auto connection = wrapper->OnStatusChange(
        [&collector](BigSegmentStoreStatus status) { collector.Add(status); });

    wrapper->Start();

    // First poll broadcasts the initial healthy status.
    ASSERT_TRUE(collector.WaitForCount(1, 1s));

    // Several more polls occur with unchanged metadata; none should broadcast.
    store->WaitForMetadataCalls(5, 1s);

    // A change flips availability and broadcasts exactly once more.
    store->SetMetadata(tl::make_unexpected("boom"));
    ASSERT_TRUE(collector.WaitForCount(2, 1s));

    // Stop the io loop before the wrapper is destroyed so no poll runs during
    // destruction.
    work_guard.reset();
    ioc.stop();
    io_thread.join();

    auto statuses = collector.Statuses();
    ASSERT_EQ(2u, statuses.size());
    EXPECT_TRUE(statuses[0].available);
    EXPECT_FALSE(statuses[1].available);
}

TEST(BigSegmentStoreWrapperMembershipTest,
     CacheMissReturnsMembershipFromStore) {
    auto store = std::make_shared<FakeBigSegmentStore>();
    store->SetMetadata(
        integrations::StoreMetadata{std::chrono::system_clock::now()});
    store->SetMembership(
        integrations::Membership::FromSegmentRefs({"segA.g1"}, {"segB.g2"}));

    auto logger = launchdarkly::logging::NullLogger();
    boost::asio::io_context ioc;
    auto wrapper = std::make_shared<BigSegmentStoreWrapper>(
        MakeConfig(store, /*poll_interval=*/5s, /*stale_after=*/2min),
        ioc.get_executor(), logger);

    auto result = wrapper->GetMembership("ctx");
    EXPECT_EQ(BigSegmentsStatus::kHealthy, result.status);
    EXPECT_EQ(true, result.membership.CheckMembership("segA.g1"));
    EXPECT_EQ(false, result.membership.CheckMembership("segB.g2"));
}

TEST(BigSegmentStoreWrapperMembershipTest, CacheHitAvoidsSecondQuery) {
    auto store = std::make_shared<FakeBigSegmentStore>();
    store->SetMetadata(
        integrations::StoreMetadata{std::chrono::system_clock::now()});
    store->SetMembership(
        integrations::Membership::FromSegmentRefs({"segA.g1"}, {}));

    auto logger = launchdarkly::logging::NullLogger();
    boost::asio::io_context ioc;
    auto wrapper = std::make_shared<BigSegmentStoreWrapper>(
        MakeConfig(store, /*poll_interval=*/5s, /*stale_after=*/2min),
        ioc.get_executor(), logger);

    auto first = wrapper->GetMembership("ctx");
    auto second = wrapper->GetMembership("ctx");
    EXPECT_EQ(true, second.membership.CheckMembership("segA.g1"));
    EXPECT_EQ(1, store->MembershipCalls());
}

TEST(BigSegmentStoreWrapperMembershipTest,
     StoreErrorIsNotCachedAndMarksUnavailable) {
    auto store = std::make_shared<FakeBigSegmentStore>();
    store->SetMetadata(
        integrations::StoreMetadata{std::chrono::system_clock::now()});
    store->SetMembership(tl::make_unexpected("boom"));

    auto logger = launchdarkly::logging::NullLogger();
    boost::asio::io_context ioc;
    auto wrapper = std::make_shared<BigSegmentStoreWrapper>(
        MakeConfig(store, /*poll_interval=*/5s, /*stale_after=*/2min),
        ioc.get_executor(), logger);

    auto result = wrapper->GetMembership("ctx");
    EXPECT_EQ(BigSegmentsStatus::kStoreError, result.status);
    EXPECT_FALSE(result.membership.CheckMembership("segA.g1").has_value());
    EXPECT_FALSE(wrapper->GetStatus().available);

    // The error was not cached, so the next lookup queries the store again.
    wrapper->GetMembership("ctx");
    EXPECT_EQ(2, store->MembershipCalls());
}

TEST(BigSegmentStoreWrapperMembershipTest, StaleStoreMakesMembershipStale) {
    auto store = std::make_shared<FakeBigSegmentStore>();
    store->SetMetadata(
        integrations::StoreMetadata{std::chrono::system_clock::now() - 5min});
    store->SetMembership(
        integrations::Membership::FromSegmentRefs({"segA.g1"}, {}));

    auto logger = launchdarkly::logging::NullLogger();
    boost::asio::io_context ioc;
    auto wrapper = std::make_shared<BigSegmentStoreWrapper>(
        MakeConfig(store, /*poll_interval=*/5s, /*stale_after=*/2min),
        ioc.get_executor(), logger);

    auto result = wrapper->GetMembership("ctx");
    EXPECT_EQ(BigSegmentsStatus::kStale, result.status);
    EXPECT_EQ(true, result.membership.CheckMembership("segA.g1"));
}

TEST(BigSegmentStoreWrapperMembershipTest, ConcurrentMissesShareOneStoreQuery) {
    auto store = std::make_shared<FakeBigSegmentStore>();
    store->SetMetadata(
        integrations::StoreMetadata{std::chrono::system_clock::now()});
    store->SetMembership(
        integrations::Membership::FromSegmentRefs({"segA.g1"}, {}));
    store->BlockMembership();

    auto logger = launchdarkly::logging::NullLogger();
    boost::asio::io_context ioc;
    auto wrapper = std::make_shared<BigSegmentStoreWrapper>(
        MakeConfig(store, /*poll_interval=*/5s, /*stale_after=*/2min),
        ioc.get_executor(), logger);

    std::optional<BigSegmentStoreWrapper::GetMembershipResult> r1;
    std::optional<BigSegmentStoreWrapper::GetMembershipResult> r2;
    std::thread t1([&] { r1 = wrapper->GetMembership("ctx"); });
    std::thread t2([&] { r2 = wrapper->GetMembership("ctx"); });

    // One caller becomes the leader and blocks in the store; while it is
    // blocked the in-flight entry persists, so the brief pause lets the other
    // caller reach the coalescing check and wait on that entry rather than
    // issuing its own query. (If coalescing were broken the follower would call
    // the store during the pause, pushing MembershipCalls to 2.)
    store->WaitForMembershipCalls(1, 1s);
    std::this_thread::sleep_for(50ms);
    store->ReleaseMembership();

    t1.join();
    t2.join();

    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(true, r1->membership.CheckMembership("segA.g1"));
    EXPECT_EQ(true, r2->membership.CheckMembership("segA.g1"));
    EXPECT_EQ(1, store->MembershipCalls());
}
