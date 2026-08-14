#pragma once

#include "fdv2_source_result.hpp"

#include <launchdarkly/async/promise.hpp>

#include <string>

namespace launchdarkly::server_side::data_interfaces {

/**
 * Defines a one-shot data source that runs to completion and returns a single
 * result. Used during the initialization phase of FDv2, before handing off to
 * an IFDv2Synchronizer.
 */
class IFDv2Initializer {
   public:
    /**
     * Returns a Future that resolves with the result once the initializer
     * completes. Called at most once per instance.
     *
     * Close() may be called from another thread to unblock Run(), in which
     * case the future resolves with FDv2SourceResult::Shutdown.
     */
    virtual async::Future<FDv2SourceResult> Run() = 0;

    /**
     * Unblocks any in-progress Run() call, causing it to return
     * FDv2SourceResult::Shutdown.
     */
    virtual void Close() = 0;

    /**
     * @return A display-suitable name of the initializer.
     */
    [[nodiscard]] virtual std::string const& Identity() const = 0;

    virtual ~IFDv2Initializer() = default;
    IFDv2Initializer(IFDv2Initializer const&) = delete;
    IFDv2Initializer(IFDv2Initializer&&) = delete;
    IFDv2Initializer& operator=(IFDv2Initializer const&) = delete;
    IFDv2Initializer& operator=(IFDv2Initializer&&) = delete;

   protected:
    IFDv2Initializer() = default;
};

}  // namespace launchdarkly::server_side::data_interfaces
