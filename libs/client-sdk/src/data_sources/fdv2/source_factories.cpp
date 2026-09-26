#include "source_factories.hpp"

#include "../polling_data_source.hpp"
#include "fdv1_adapter_synchronizer.hpp"
#include "polling_initializer.hpp"
#include "polling_synchronizer.hpp"
#include "streaming_synchronizer.hpp"

#include <utility>

namespace launchdarkly::client_side::data_sources {

FDv2PollingInitializerFactory::FDv2PollingInitializerFactory(
    boost::asio::any_io_executor executor,
    Logger logger,
    FDv2RequestConfig request_config)
    : executor_(std::move(executor)),
      logger_(std::move(logger)),
      request_config_(std::move(request_config)) {}

std::unique_ptr<IFDv2Initializer> FDv2PollingInitializerFactory::Build() {
    return std::make_unique<FDv2PollingInitializer>(executor_, logger_,
                                                    request_config_);
}

FDv2PollingSynchronizerFactory::FDv2PollingSynchronizerFactory(
    boost::asio::any_io_executor executor,
    Logger logger,
    FDv2RequestConfig request_config,
    std::chrono::seconds poll_interval,
    std::optional<std::chrono::steady_clock::time_point> last_poll)
    : executor_(std::move(executor)),
      logger_(std::move(logger)),
      request_config_(std::move(request_config)),
      poll_interval_(poll_interval),
      last_poll_(last_poll) {}

std::unique_ptr<IFDv2Synchronizer> FDv2PollingSynchronizerFactory::Build() {
    return std::make_unique<FDv2PollingSynchronizer>(
        executor_, logger_, request_config_, poll_interval_, last_poll_);
}

FDv2StreamingSynchronizerFactory::FDv2StreamingSynchronizerFactory(
    boost::asio::any_io_executor executor,
    Logger logger,
    FDv2RequestConfig stream_config,
    FDv2RequestConfig poll_config,
    std::chrono::milliseconds initial_reconnect_delay)
    : executor_(std::move(executor)),
      logger_(std::move(logger)),
      stream_config_(std::move(stream_config)),
      poll_config_(std::move(poll_config)),
      initial_reconnect_delay_(initial_reconnect_delay) {}

std::unique_ptr<IFDv2Synchronizer> FDv2StreamingSynchronizerFactory::Build() {
    return std::make_unique<FDv2StreamingSynchronizer>(
        executor_, logger_, stream_config_, poll_config_,
        initial_reconnect_delay_);
}

FDv1PollingAdapterFactory::FDv1PollingAdapterFactory(
    boost::asio::any_io_executor executor,
    Logger logger,
    config::shared::built::ServiceEndpoints endpoints,
    config::shared::built::DataSourceConfig<config::shared::ClientSDK>
        data_source_config,
    config::shared::built::HttpProperties http_properties,
    Context context)
    : executor_(std::move(executor)),
      logger_(std::move(logger)),
      endpoints_(std::move(endpoints)),
      data_source_config_(std::move(data_source_config)),
      http_properties_(std::move(http_properties)),
      context_(std::move(context)) {}

std::unique_ptr<IFDv2Synchronizer> FDv1PollingAdapterFactory::Build() {
    return std::make_unique<FDv1AdapterSynchronizer>(
        [this](IDataSourceUpdateSink* sink,
               DataSourceStatusManager* status_manager) {
            return std::make_shared<PollingDataSource>(
                endpoints_, data_source_config_, http_properties_, executor_,
                context_, *sink, *status_manager, logger_);
        });
}

}  // namespace launchdarkly::client_side::data_sources
