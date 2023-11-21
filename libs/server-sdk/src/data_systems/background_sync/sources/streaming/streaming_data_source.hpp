#pragma once

#include "event_handler.hpp"

#include "../../../../data_components/status_notifications/data_source_status_manager.hpp"
#include "../../../../data_interfaces/destination/idestination.hpp"
#include "../../../../data_interfaces/source/idata_synchronizer.hpp"

#include <launchdarkly/server_side/config/built/all_built.hpp>

#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/sse/client.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <chrono>

using namespace std::chrono_literals;

namespace launchdarkly::server_side::data_systems {

class StreamingDataSource final
    : public data_interfaces::IDataSynchronizer,
      public std::enable_shared_from_this<StreamingDataSource> {
   public:
    StreamingDataSource(
        config::built::ServiceEndpoints const& endpoints,
        config::built::BackgroundSyncConfig::StreamingConfig const&
            data_source_config,
        config::built::HttpProperties http_properties,
        boost::asio::any_io_executor ioc,
        data_interfaces::IDestination& handler,
        data_components::DataSourceStatusManager& status_manager,
        Logger const& logger);

    void Init(std::optional<data_model::SDKDataSet> initial_data) override;
    void StartAsync() override;
    void ShutdownAsync(std::function<void()> completion) override;

    [[nodiscard]] std::string const& Identity() const override;

   private:
    boost::asio::any_io_executor exec_;
    data_components::DataSourceStatusManager& status_manager_;
    DataSourceEventHandler data_source_handler_;
    std::string streaming_endpoint_;

    config::built::BackgroundSyncConfig::StreamingConfig streaming_config_;

    config::built::HttpProperties http_config_;

    Logger const& logger_;
    std::shared_ptr<sse::Client> client_;
};
}  // namespace launchdarkly::server_side::data_systems
