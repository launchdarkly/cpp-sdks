#pragma once

#include "ifdv2_initializer.hpp"

#include <memory>

namespace launchdarkly::client_side::data_sources {

/**
 * Builds new IFDv2Initializer instances on demand. Each call to Build()
 * produces a fresh initializer that has not yet been started.
 *
 * Implementations must be thread-safe. Build() and IsFromCache() may be
 * called from any thread.
 */
class IFDv2InitializerFactory {
   public:
    virtual std::unique_ptr<IFDv2Initializer> Build() = 0;

    /**
     * Whether the initializers this factory builds read from the local cache
     * rather than the network. When the cache is the only possible source of
     * data, a miss still completes initialization successfully, so the
     * orchestrator needs to tell the two apart.
     */
    [[nodiscard]] virtual bool IsFromCache() const { return false; }

    virtual ~IFDv2InitializerFactory() = default;
    IFDv2InitializerFactory(IFDv2InitializerFactory const&) = delete;
    IFDv2InitializerFactory(IFDv2InitializerFactory&&) = delete;
    IFDv2InitializerFactory& operator=(IFDv2InitializerFactory const&) = delete;
    IFDv2InitializerFactory& operator=(IFDv2InitializerFactory&&) = delete;

   protected:
    IFDv2InitializerFactory() = default;
};

}  // namespace launchdarkly::client_side::data_sources
