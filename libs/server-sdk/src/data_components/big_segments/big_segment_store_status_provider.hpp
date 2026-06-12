#pragma once

#include "big_segment_store_wrapper.hpp"

#include <launchdarkly/server_side/big_segment_store_status.hpp>

#include <functional>
#include <memory>

namespace launchdarkly::server_side::data_components {

/**
 * @brief Adapts a BigSegmentStoreWrapper to the public
 * IBigSegmentStoreStatusProvider, converting the internal status struct to the
 * public type.
 *
 * Holds a null wrapper when Big Segments are not configured; in that case it
 * reports the store as unavailable and not stale, and registered listeners
 * never fire.
 *
 * Thread-safe: delegates to the wrapper, which is itself thread-safe.
 *
 * Note: the public status type server_side::BigSegmentStoreStatus is qualified
 * throughout, since this namespace also has an internal struct of the same
 * name.
 */
class BigSegmentStoreStatusProvider : public IBigSegmentStoreStatusProvider {
   public:
    /**
     * @param wrapper The wrapper to delegate to, or nullptr if Big Segments are
     * not configured.
     */
    explicit BigSegmentStoreStatusProvider(
        std::shared_ptr<BigSegmentStoreWrapper> wrapper);

    [[nodiscard]] server_side::BigSegmentStoreStatus Status() const override;

    std::unique_ptr<IConnection> OnBigSegmentStoreStatusChange(
        std::function<void(server_side::BigSegmentStoreStatus status)> handler)
        override;

   private:
    std::shared_ptr<BigSegmentStoreWrapper> wrapper_;
};

}  // namespace launchdarkly::server_side::data_components
