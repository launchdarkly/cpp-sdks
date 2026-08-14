#pragma once

#include "big_segments_status.hpp"
#include "membership_cache.hpp"

#include <launchdarkly/server_side/config/built/big_segments_config.hpp>
#include <launchdarkly/server_side/integrations/big_segments/ibig_segment_store.hpp>

#include <launchdarkly/async/cancellation.hpp>
#include <launchdarkly/connection.hpp>
#include <launchdarkly/logging/logger.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/signals2/signal.hpp>

#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_components {

/**
 * @brief Internal layer between the evaluator and a customer-provided
 * @ref integrations::IBigSegmentStore.
 *
 * Adds context-key hashing, an LRU membership cache, a background task that
 * polls the store's metadata to track availability and staleness, and a
 * status-change broadcaster.
 *
 * Construct with @ref std::make_shared and call @ref Start once to begin
 * background polling; polling stops when the wrapper is destroyed.
 *
 * Thread-safe: @ref GetMembership, @ref GetStatus, and @ref OnStatusChange may
 * be called concurrently from any number of evaluation threads, including while
 * the background poll runs on the executor. The store is only ever queried with
 * the relevant lock released, so a slow store never blocks unrelated callers.
 */
class BigSegmentStoreWrapper
    : public std::enable_shared_from_this<BigSegmentStoreWrapper> {
   public:
    /**
     * @param config Resolved Big Segments configuration.
     * @param executor Executor the background poll runs on.
     * @param logger Used for store-error and debug logging. Must outlive the
     * wrapper.
     */
    BigSegmentStoreWrapper(config::built::BigSegmentsConfig const& config,
                           boost::asio::any_io_executor executor,
                           Logger const& logger);

    ~BigSegmentStoreWrapper();

    BigSegmentStoreWrapper(BigSegmentStoreWrapper const&) = delete;
    BigSegmentStoreWrapper(BigSegmentStoreWrapper&&) = delete;
    BigSegmentStoreWrapper& operator=(BigSegmentStoreWrapper const&) = delete;
    BigSegmentStoreWrapper& operator=(BigSegmentStoreWrapper&&) = delete;

    /**
     * @brief Begins periodic metadata polling on the executor. Call once after
     * construction.
     */
    void Start();

    struct GetMembershipResult {
        integrations::Membership membership;
        BigSegmentsStatus status;
    };

    /**
     * @brief Returns a context's Big Segments membership, with a status
     * describing how trustworthy the answer is.
     *
     * May be called concurrently from multiple evaluation threads. If the
     * status is @ref BigSegmentsStatus::kStoreError the lookup failed and the
     * returned membership is empty.
     *
     * @param context_key The unhashed context key.
     */
    [[nodiscard]] GetMembershipResult GetMembership(
        std::string const& context_key);

    /**
     * @brief Returns the current store health. If no metadata poll has
     * completed yet, performs one synchronously on the calling thread first, so
     * the first caller still gets an accurate staleness reading.
     */
    [[nodiscard]] BigSegmentStoreStatus GetStatus();

    /**
     * @brief Registers a listener invoked whenever the store status changes
     * (not on every poll). Call Disconnect() on the returned connection to
     * stop receiving events.
     */
    std::unique_ptr<IConnection> OnStatusChange(
        std::function<void(BigSegmentStoreStatus)> handler);

   private:
    using StoreMembership = integrations::IBigSegmentStore::GetMembershipResult;

    // A store query shared by all callers that miss the same key concurrently:
    // the leader fills result and notifies; waiters block on cv until then.
    struct InFlightQuery {
        std::mutex mutex;
        std::condition_variable cv;
        std::optional<StoreMembership> result;
    };

    // Returns the membership for a key, querying the store on a cache miss and
    // coalescing concurrent misses of the same key into one query.
    [[nodiscard]] StoreMembership LoadMembership(
        std::string const& context_key);

    // Marks the store unavailable after an evaluation-time error, preserving
    // the last-known staleness, and broadcasts if the status changed.
    void MarkStoreUnavailable();

    // Queries store metadata, atomically updates the status, broadcasts if it
    // changed, and returns the new status.
    BigSegmentStoreStatus PollStoreAndUpdateStatus();

    [[nodiscard]] bool IsStale(
        std::chrono::system_clock::time_point last_up_to_date) const;

    // Arms a one-shot delay that polls then re-arms itself, until cancelled.
    void ScheduleNextPoll();

    std::shared_ptr<integrations::IBigSegmentStore> const store_;
    std::chrono::milliseconds const stale_after_;
    std::chrono::milliseconds const poll_interval_;
    Logger const& logger_;

    boost::asio::any_io_executor executor_;

    // Cancels the pending poll delay on destruction, ending the poll loop.
    async::CancellationSource poll_cancel_;

    // Internally thread-safe.
    MembershipCache cache_;

    // Broadcasts status changes to listeners registered via OnStatusChange.
    boost::signals2::signal<void(BigSegmentStoreStatus)> status_signal_;

    // Coalesces the cold-start fallback poll in GetStatus().
    std::once_flag first_poll_once_;

    // In-flight store queries keyed by context key. Protected by load_mutex_.
    std::mutex load_mutex_;
    std::unordered_map<std::string, std::shared_ptr<InFlightQuery>> in_flight_;

    // Protected by status_mutex_; nullopt until the first poll completes.
    std::mutex status_mutex_;
    std::optional<BigSegmentStoreStatus> last_status_;
};

}  // namespace launchdarkly::server_side::data_components
