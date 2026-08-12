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
    PollingDataSource(boost::asio::any_io_executor const& ioc,
                      Logger const& logger,
                      data_components::DataSourceStatusManager& status_manager,
                      config::built::ServiceEndpoints const& endpoints,
                      config::built::BackgroundSyncConfig::PollingConfig const&
                          data_source_config,
                      config::built::HttpProperties const& http_properties);

    void StartAsync(data_interfaces::IDestination* dest,
                    data_model::SDKDataSet const* bootstrap_data) override;

    void ShutdownAsync(std::function<void()> completion) override;

    [[nodiscard]] std::string const& Identity() const override;

   private:
    void DoPoll();
    void HandlePollResult(network::HttpResult const& res);

    Logger const& logger_;

    // Status manager is used to report the status of the data source. It must
    // outlive the source. This source performs asynchronous
    // operations, so a completion handler might invoke the status manager after
    // it has been destroyed.
    data_components::DataSourceStatusManager& status_manager_;

    // Responsible for performing HTTP requests using boost::asio.
    network::AsioRequester requester_;

    // How long to wait beteween individual polling requests.
    std::chrono::seconds polling_interval_;

    // Cached request arguments used in the polling request.
    network::HttpRequest request_;

    // Etag can be sent in HTTP request to save bandwidth if the server knows
    // the response is unchanged.
    std::optional<std::string> etag_;

    // Used with polling_interval to schedule polling requests.
    boost::asio::steady_timer timer_;

    // The last time the polling HTTP request is initiated.
    std::chrono::time_point<std::chrono::system_clock> last_poll_start_;

    // Destination for all data obtained via polling.
    data_interfaces::IDestination* sink_;

    void StartPollingTimer();
};

}  // namespace launchdarkly::server_side::data_systems
