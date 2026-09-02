#pragma once

#include "fdv2_source_result.hpp"

#include <launchdarkly/async/promise.hpp>
#include <launchdarkly/data_model/selector.hpp>

#include <string>

namespace launchdarkly::client_side::data_sources {

/**
 * A continuous data source that produces a stream of results, used to keep
 * flag data current once initialization is complete.
 *
 * The underlying connection is started lazily on the first call to Next() and
 * runs until Close() is called.
 *
 * Implementations must be thread-safe to the extent this contract needs:
 * Next() is called from one thread at a time, and Close() may be called
 * concurrently with it from another.
 */
class IFDv2Synchronizer {
   public:
    /**
     * Returns a Future that resolves with the next result once it is
     * available.
     *
     * On the first call, the synchronizer starts its underlying connection.
     * Subsequent calls continue reading from the same connection.
     *
     * Close() may be called from another thread to unblock Next(), in which
     * case the future resolves with FDv2SourceResult::Shutdown.
     *
     * @param selector The selector to send with the request, reflecting any
     * changesets applied since the previous call. An empty selector asks for
     * a full data set.
     */
    virtual async::Future<FDv2SourceResult> Next(
        data_model::Selector selector) = 0;

    /**
     * Unblocks any in-progress Next() call, causing it to return
     * FDv2SourceResult::Shutdown, and releases underlying resources.
     */
    virtual void Close() = 0;

    /**
     * @return A display-suitable name of the synchronizer.
     */
    [[nodiscard]] virtual std::string const& Identity() const = 0;

    virtual ~IFDv2Synchronizer() = default;
    IFDv2Synchronizer(IFDv2Synchronizer const&) = delete;
    IFDv2Synchronizer(IFDv2Synchronizer&&) = delete;
    IFDv2Synchronizer& operator=(IFDv2Synchronizer const&) = delete;
    IFDv2Synchronizer& operator=(IFDv2Synchronizer&&) = delete;

   protected:
    IFDv2Synchronizer() = default;
};

}  // namespace launchdarkly::client_side::data_sources
