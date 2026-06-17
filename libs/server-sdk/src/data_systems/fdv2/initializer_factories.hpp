#pragma once

#include "../../data_interfaces/source/ifdv2_initializer_factory.hpp"

#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/server_side/config/built/all_built.hpp>
#include <launchdarkly/server_side/config/built/data_system/fdv2_config.hpp>

#include <boost/asio/any_io_executor.hpp>

namespace launchdarkly::server_side::data_systems {

/**
 * Builds fresh FDv2PollingInitializer instances on demand.
 */
class FDv2PollingInitializerFactory final
    : public data_interfaces::IFDv2InitializerFactory {
   public:
    FDv2PollingInitializerFactory(
        boost::asio::any_io_executor executor,
        Logger logger,
        config::built::ServiceEndpoints endpoints,
        config::built::HttpProperties http_properties,
        config::built::FDv2Config::PollingConfig polling);

    std::unique_ptr<data_interfaces::IFDv2Initializer> Build() override;

   private:
    boost::asio::any_io_executor const executor_;
    Logger const logger_;
    std::string const polling_base_url_;
    config::built::HttpProperties const http_properties_;
    config::built::FDv2Config::PollingConfig const polling_;
};

}  // namespace launchdarkly::server_side::data_systems
