#pragma once

#include "fdv2_source_result.hpp"

#include <chrono>
#include <string>

namespace launchdarkly::server_side::data_interfaces {

/**
 * Defines a continuous data source that produces a stream of results. Used
 * during the synchronization phase of FDv2, after initialization is complete.
 *
 * The stream is started lazily on the first call to Next(). The synchronizer
 * runs until Close() is called.
 */
class IFDv2Synchronizer {
   public:
    /**
     * Block until the next result is available or the timeout expires.
     *
     * On the first call, the synchronizer starts its underlying connection.
     * Subsequent calls continue reading from the same connection.
     *
     * If the timeout expires before a result arrives, returns
     * FDv2SourceResult::Interrupted. The orchestrator uses this to evaluate
     * fallback conditions.
     *
     * Close() may be called from another thread to unblock Next(), in which
     * case Next() returns FDv2SourceResult::Shutdown.
     *
     * @param timeout Maximum time to wait for the next result.
     */
    virtual FDv2SourceResult Next(std::chrono::milliseconds timeout) = 0;

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

}  // namespace launchdarkly::server_side::data_interfaces
