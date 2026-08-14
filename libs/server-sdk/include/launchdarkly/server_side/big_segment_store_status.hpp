#pragma once

#include <launchdarkly/connection.hpp>

#include <functional>
#include <memory>
#include <ostream>

namespace launchdarkly::server_side {

/**
 * The current health of a Big Segments store, independent of any single
 * context's membership.
 */
class BigSegmentStoreStatus {
   public:
    BigSegmentStoreStatus(bool available, bool stale);

    /**
     * True if the most recent store query or metadata poll succeeded. If false,
     * Big Segments membership cannot currently be evaluated reliably.
     */
    [[nodiscard]] bool IsAvailable() const;

    /**
     * True if the store's data has not been updated within the configured
     * stale-after threshold. The data may still be queried; it is just older
     * than desired.
     */
    [[nodiscard]] bool IsStale() const;

   private:
    bool available_;
    bool stale_;
};

bool operator==(BigSegmentStoreStatus const& a, BigSegmentStoreStatus const& b);
bool operator!=(BigSegmentStoreStatus const& a, BigSegmentStoreStatus const& b);

/**
 * Interface for accessing and listening to the Big Segments store status.
 */
class IBigSegmentStoreStatusProvider {
   public:
    /**
     * The current status of the Big Segments store. If no store is configured,
     * reports unavailable and not stale.
     */
    [[nodiscard]] virtual BigSegmentStoreStatus Status() const = 0;

    /**
     * Listen to changes to the Big Segments store status. The handler is
     * invoked only when the status changes, not on every metadata poll.
     *
     * @param handler Function which will be called with the new status.
     * @return A IConnection which can be used to stop listening to the status.
     */
    virtual std::unique_ptr<IConnection> OnBigSegmentStoreStatusChange(
        std::function<void(BigSegmentStoreStatus status)> handler) = 0;

    virtual ~IBigSegmentStoreStatusProvider() = default;
    IBigSegmentStoreStatusProvider(IBigSegmentStoreStatusProvider const&) =
        delete;
    IBigSegmentStoreStatusProvider(IBigSegmentStoreStatusProvider&&) = delete;
    IBigSegmentStoreStatusProvider& operator=(
        IBigSegmentStoreStatusProvider const&) = delete;
    IBigSegmentStoreStatusProvider& operator=(
        IBigSegmentStoreStatusProvider&&) = delete;

   protected:
    IBigSegmentStoreStatusProvider() = default;
};

std::ostream& operator<<(std::ostream& out,
                         BigSegmentStoreStatus const& status);

}  // namespace launchdarkly::server_side
