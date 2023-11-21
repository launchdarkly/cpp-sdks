#pragma once

#include "../../../../data_components/status_notifications/data_source_status_manager.hpp"
#include "../../../../data_interfaces/destination/idestination.hpp"
#include "../../../../data_interfaces/source/idata_synchronizer.hpp"

#include <launchdarkly/server_side/config/built/all_built.hpp>

#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/network/asio_requester.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <chrono>

namespace launchdarkly::server_side::data_systems {

class PollingDataSource
    : public data_interfaces::IDataSynchronizer,
      public std::enable_shared_from_this<PollingDataSource> {
   public:
    PollingDataSource(config::built::ServiceEndpoints const& endpoints,
                      config::built::BackgroundSyncConfig::PollingConfig const&
                          data_source_config,
                      config::built::HttpProperties const& http_properties,
                      boost::asio::any_io_executor const& ioc,
                      data_interfaces::IDestination& handler,
                      data_components::DataSourceStatusManager& status_manager,
                      Logger const& logger);

    void Init(std::optional<data_model::SDKDataSet> initial_data) override;
    void StartAsync() override;
    void ShutdownAsync(std::function<void()> completion) override;

    [[nodiscard]] std::string const& Identity() const override;

   private:
    void DoPoll();
    void HandlePollResult(network::HttpResult const& res);

    data_components::DataSourceStatusManager& status_manager_;
    std::string polling_endpoint_;

    network::AsioRequester requester_;
    Logger const& logger_;
    boost::asio::any_io_executor ioc_;
    std::chrono::seconds polling_interval_;
    network::HttpRequest request_;
    std::optional<std::string> etag_;

    boost::asio::steady_timer timer_;
    std::chrono::time_point<std::chrono::system_clock> last_poll_start_;
    data_interfaces::IDestination& update_sink_;

    void StartPollingTimer();
};

}  // namespace launchdarkly::server_side::data_systems
