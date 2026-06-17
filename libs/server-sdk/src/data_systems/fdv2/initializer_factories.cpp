#include "initializer_factories.hpp"

#include "polling_initializer.hpp"

#include <launchdarkly/data_model/selector.hpp>

#include <utility>

namespace launchdarkly::server_side::data_systems {

FDv2PollingInitializerFactory::FDv2PollingInitializerFactory(
    boost::asio::any_io_executor executor,
    Logger logger,
    config::built::ServiceEndpoints endpoints,
    config::built::HttpProperties http_properties,
    config::built::FDv2Config::PollingConfig polling)
    : executor_(std::move(executor)),
      logger_(std::move(logger)),
      polling_base_url_(
          polling.base_url_override.value_or(endpoints.PollingBaseUrl())),
      http_properties_(std::move(http_properties)),
      polling_(std::move(polling)) {}

std::unique_ptr<data_interfaces::IFDv2Initializer>
FDv2PollingInitializerFactory::Build() {
    return std::make_unique<FDv2PollingInitializer>(
        executor_, logger_, polling_base_url_, http_properties_,
        data_model::Selector{}, std::nullopt);
}

}  // namespace launchdarkly::server_side::data_systems
