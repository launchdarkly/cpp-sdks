#include "synchronizer_factories.hpp"

#include "../background_sync/sources/polling/polling_data_source.hpp"
#include "../background_sync/sources/streaming/streaming_data_source.hpp"
#include "fdv1_adapter_synchronizer.hpp"
#include "polling_synchronizer.hpp"
#include "streaming_synchronizer.hpp"

#include <launchdarkly/config/shared/defaults.hpp>

#include <utility>

namespace launchdarkly::server_side::data_systems {

FDv2StreamingSynchronizerFactory::FDv2StreamingSynchronizerFactory(
    boost::asio::any_io_executor executor,
    Logger logger,
    config::built::ServiceEndpoints endpoints,
    config::built::HttpProperties http_properties,
    config::built::FDv2Config::StreamingConfig streaming)
    : executor_(std::move(executor)),
      logger_(std::move(logger)),
      streaming_base_url_(
          streaming.base_url_override.value_or(endpoints.StreamingBaseUrl())),
      http_properties_(std::move(http_properties)),
      streaming_(std::move(streaming)) {}

std::unique_ptr<data_interfaces::IFDv2Synchronizer>
FDv2StreamingSynchronizerFactory::Build() {
    return std::make_unique<FDv2StreamingSynchronizer>(
        executor_, logger_, streaming_base_url_, http_properties_, std::nullopt,
        streaming_.initial_reconnect_delay);
}

FDv2PollingSynchronizerFactory::FDv2PollingSynchronizerFactory(
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

std::unique_ptr<data_interfaces::IFDv2Synchronizer>
FDv2PollingSynchronizerFactory::Build() {
    return std::make_unique<FDv2PollingSynchronizer>(
        executor_, logger_, polling_base_url_, http_properties_, std::nullopt,
        polling_.poll_interval);
}

FDv1StreamingAdapterFactory::FDv1StreamingAdapterFactory(
    boost::asio::any_io_executor executor,
    Logger logger,
    config::built::ServiceEndpoints endpoints,
    config::built::FDv2Config::FDv1StreamingConfig streaming,
    config::built::HttpProperties http_properties)
    : executor_(std::move(executor)),
      logger_(std::move(logger)),
      endpoints_(std::move(endpoints)),
      streaming_(std::move(streaming)),
      http_properties_(std::move(http_properties)) {}

std::unique_ptr<data_interfaces::IFDv2Synchronizer>
FDv1StreamingAdapterFactory::Build() {
    return std::make_unique<FDv1AdapterSynchronizer>(
        [this](data_components::DataSourceStatusManager& status_manager) {
            return std::make_shared<StreamingDataSource>(
                executor_, logger_, status_manager, endpoints_, streaming_,
                http_properties_);
        });
}

FDv1PollingAdapterFactory::FDv1PollingAdapterFactory(
    boost::asio::any_io_executor executor,
    Logger logger,
    config::built::ServiceEndpoints endpoints,
    config::built::FDv2Config::FDv1PollingConfig polling,
    config::built::HttpProperties http_properties)
    : executor_(std::move(executor)),
      logger_(std::move(logger)),
      endpoints_(std::move(endpoints)),
      polling_(std::move(polling)),
      http_properties_(std::move(http_properties)) {}

std::unique_ptr<data_interfaces::IFDv2Synchronizer>
FDv1PollingAdapterFactory::Build() {
    return std::make_unique<FDv1AdapterSynchronizer>(
        [this](data_components::DataSourceStatusManager& status_manager) {
            return std::make_shared<PollingDataSource>(
                executor_, logger_, status_manager, endpoints_, polling_,
                http_properties_);
        });
}

}  // namespace launchdarkly::server_side::data_systems
