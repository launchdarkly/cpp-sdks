#include "synchronizer_factories.hpp"

#include "../background_sync/sources/streaming/streaming_data_source.hpp"
#include "fdv1_adapter_synchronizer.hpp"
#include "polling_synchronizer.hpp"
#include "streaming_synchronizer.hpp"

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
      endpoints_(std::move(endpoints)),
      http_properties_(std::move(http_properties)),
      streaming_(std::move(streaming)) {}

std::unique_ptr<data_interfaces::IFDv2Synchronizer>
FDv2StreamingSynchronizerFactory::Build() {
    return std::make_unique<FDv2StreamingSynchronizer>(
        executor_, logger_, endpoints_, http_properties_, streaming_.filter_key,
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
      endpoints_(std::move(endpoints)),
      http_properties_(std::move(http_properties)),
      polling_(std::move(polling)) {}

std::unique_ptr<data_interfaces::IFDv2Synchronizer>
FDv2PollingSynchronizerFactory::Build() {
    return std::make_unique<FDv2PollingSynchronizer>(
        executor_, logger_, endpoints_, http_properties_, polling_.filter_key,
        polling_.poll_interval);
}

FDv1StreamingAdapterFactory::FDv1StreamingAdapterFactory(
    boost::asio::any_io_executor executor,
    Logger logger,
    data_components::DataSourceStatusManager* status_manager,
    config::built::ServiceEndpoints endpoints,
    config::built::FDv2Config::StreamingConfig streaming,
    config::built::HttpProperties http_properties)
    : executor_(std::move(executor)),
      logger_(std::move(logger)),
      status_manager_(status_manager),
      endpoints_(std::move(endpoints)),
      streaming_(std::move(streaming)),
      http_properties_(std::move(http_properties)) {}

std::unique_ptr<data_interfaces::IFDv2Synchronizer>
FDv1StreamingAdapterFactory::Build() {
    auto fdv1_source = std::make_unique<StreamingDataSource>(
        executor_, logger_, *status_manager_, endpoints_, streaming_,
        http_properties_);
    return std::make_unique<FDv1AdapterSynchronizer>(std::move(fdv1_source));
}

}  // namespace launchdarkly::server_side::data_systems
