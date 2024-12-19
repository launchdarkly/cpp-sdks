#pragma once

#include "event_handler.hpp"

#include "../../../../data_components/status_notifications/data_source_status_manager.hpp"
#include "../../../../data_interfaces/destination/idestination.hpp"
#include "../../../../data_interfaces/source/idata_synchronizer.hpp"

#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/server_side/config/built/all_built.hpp>
#include <launchdarkly/sse/client.hpp>

#include <boost/asio/any_io_executor.hpp>

namespace launchdarkly::server_side::data_systems {

class StreamingDataSource final
    : public data_interfaces::IDataSynchronizer,
      public std::enable_shared_from_this<StreamingDataSource> {
   public:
    StreamingDataSource(
        boost::asio::any_io_executor io,
        Logger const& logger,
        data_components::DataSourceStatusManager& status_manager,
        config::built::ServiceEndpoints const& endpoints,
        config::built::BackgroundSyncConfig::StreamingConfig const& streaming,
        config::built::HttpProperties const& http_properties);

    void StartAsync(data_interfaces::IDestination* dest,
                    data_model::SDKDataSet const* bootstrap_data) override;
    void ShutdownAsync(std::function<void()> completion) override;

    [[nodiscard]] std::string const& Identity() const override;

   private:
    void HandleErrorStateChange(sse::Error error, std::string error_string);

    boost::asio::any_io_executor io_;
    Logger const& logger_;

    data_components::DataSourceStatusManager& status_manager_;
    config::built::HttpProperties http_config_;

    std::optional<DataSourceEventHandler> event_handler_;
    std::string streaming_endpoint_;

    config::built::BackgroundSyncConfig::StreamingConfig streaming_config_;

    std::shared_ptr<sse::Client> client_;
};
}  // namespace launchdarkly::server_side::data_systems
