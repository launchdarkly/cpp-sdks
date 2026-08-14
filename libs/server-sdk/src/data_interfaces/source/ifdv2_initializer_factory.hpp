#pragma once

#include "ifdv2_initializer.hpp"

#include <memory>

namespace launchdarkly::server_side::data_interfaces {

/**
 * Builds new IFDv2Initializer instances on demand. Each call to Build()
 * produces a fresh initializer that has not yet been started.
 */
class IFDv2InitializerFactory {
   public:
    virtual std::unique_ptr<IFDv2Initializer> Build() = 0;

    virtual ~IFDv2InitializerFactory() = default;
    IFDv2InitializerFactory(IFDv2InitializerFactory const&) = delete;
    IFDv2InitializerFactory(IFDv2InitializerFactory&&) = delete;
    IFDv2InitializerFactory& operator=(IFDv2InitializerFactory const&) = delete;
    IFDv2InitializerFactory& operator=(IFDv2InitializerFactory&&) = delete;

   protected:
    IFDv2InitializerFactory() = default;
};

}  // namespace launchdarkly::server_side::data_interfaces
