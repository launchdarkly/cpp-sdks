#pragma once

#include "ifdv2_synchronizer.hpp"

#include <memory>

namespace launchdarkly::client_side::data_sources {

/**
 * Builds new IFDv2Synchronizer instances on demand. Each call to Build()
 * produces a fresh synchronizer that has not yet been started.
 *
 * Implementations must be thread-safe. Build() and IsFDv1Fallback() may be
 * called from any thread.
 */
class IFDv2SynchronizerFactory {
   public:
    virtual std::unique_ptr<IFDv2Synchronizer> Build() = 0;

    /**
     * Whether the synchronizers this factory builds speak FDv1.
     */
    [[nodiscard]] virtual bool IsFDv1Fallback() const { return false; }

    virtual ~IFDv2SynchronizerFactory() = default;
    IFDv2SynchronizerFactory(IFDv2SynchronizerFactory const&) = delete;
    IFDv2SynchronizerFactory(IFDv2SynchronizerFactory&&) = delete;
    IFDv2SynchronizerFactory& operator=(IFDv2SynchronizerFactory const&) =
        delete;
    IFDv2SynchronizerFactory& operator=(IFDv2SynchronizerFactory&&) = delete;

   protected:
    IFDv2SynchronizerFactory() = default;
};

}  // namespace launchdarkly::client_side::data_sources
