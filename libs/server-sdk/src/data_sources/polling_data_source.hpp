#pragma once

#include <chrono>

#include <boost/asio/any_io_executor.hpp>

#include "data_source_event_handler.hpp"
#include "data_source_status_manager.hpp"
#include "data_source_update_sink.hpp"

#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/data_sources/data_source.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/network/asio_requester.hpp>

namespace launchdarkly::server_side::data_sources {

class PollingDataSource
    : public ::launchdarkly::data_sources::IDataSource,
      public std::enable_shared_from_this<PollingDataSource> {
   public:
    PollingDataSource(
        config::shared::built::ServiceEndpoints const& endpoints,
        config::shared::built::DataSourceConfig<
            config::shared::ServerSDK> const& data_source_config,
        config::shared::built::HttpProperties const& http_properties,
        boost::asio::any_io_executor const& ioc,
        IDataSourceUpdateSink& handler,
        DataSourceStatusManager& status_manager,
        Logger const& logger);

    void Start() override;
    void ShutdownAsync(std::function<void()> completion) override;

   private:
    void DoPoll();
    void HandlePollResult(network::HttpResult const& res);

    DataSourceStatusManager& status_manager_;
    std::string polling_endpoint_;

    network::AsioRequester requester_;
    Logger const& logger_;
    boost::asio::any_io_executor ioc_;
    std::chrono::seconds polling_interval_;
    network::HttpRequest request_;
    std::optional<std::string> etag_;

    boost::asio::steady_timer timer_;
    std::chrono::time_point<std::chrono::system_clock> last_poll_start_;
    IDataSourceUpdateSink& update_sink_;

    void StartPollingTimer();
};

}  // namespace launchdarkly::server_side::data_sources