#pragma once

#include "ifdv2_synchronizer.hpp"

#include <memory>

namespace launchdarkly::server_side::data_interfaces {

/**
 * Builds new IFDv2Synchronizer instances on demand. Each call to Build()
 * produces a fresh synchronizer that has not yet been started.
 */
class IFDv2SynchronizerFactory {
   public:
    virtual std::unique_ptr<IFDv2Synchronizer> Build() = 0;

    virtual ~IFDv2SynchronizerFactory() = default;
    IFDv2SynchronizerFactory(IFDv2SynchronizerFactory const&) = delete;
    IFDv2SynchronizerFactory(IFDv2SynchronizerFactory&&) = delete;
    IFDv2SynchronizerFactory& operator=(IFDv2SynchronizerFactory const&) =
        delete;
    IFDv2SynchronizerFactory& operator=(IFDv2SynchronizerFactory&&) = delete;

   protected:
    IFDv2SynchronizerFactory() = default;
};

}  // namespace launchdarkly::server_side::data_interfaces
