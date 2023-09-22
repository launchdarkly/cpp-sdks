#pragma once

#include <chrono>
using namespace std::chrono_literals;

#include <boost/asio/any_io_executor.hpp>

#include "../../interfaces/data_dest/data_destination.hpp"
#include "../../interfaces/data_source/data_push_source.hpp"

#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/config/shared/sdks.hpp>
#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_result.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/sse/client.hpp>

namespace launchdarkly::server_side::data_system {

class StreamingDataSource final
    : public IDataPushSource,
      public std::enable_shared_from_this<StreamingDataSource> {
   public:
    StreamingDataSource(
        config::shared::built::ServiceEndpoints const& endpoints,
        config::shared::built::StreamingConfig<config::shared::ServerSDK> const&
            data_source_config,
        config::shared::built::HttpProperties http_properties,
        boost::asio::any_io_executor ioc,
        IDataSourceUpdateSink& handler,
        DataSourceStatusManager& status_manager,
        Logger const& logger);

    void Init(std::optional<data_model::SDKDataSet> initial_data,
              IDataDestination& destination) override;
    void Start() override;
    void ShutdownAsync(std::function<void()> completion) override;

   private:
    boost::asio::any_io_executor exec_;
    DataSourceStatusManager& status_manager_;
    DataSourceEventHandler data_source_handler_;
    std::string streaming_endpoint_;

    config::shared::built::StreamingConfig<config::shared::ServerSDK>
        streaming_config_;

    config::shared::built::HttpProperties http_config_;

    Logger const& logger_;
    std::shared_ptr<launchdarkly::sse::Client> client_;
};
}  // namespace launchdarkly::server_side::data_system
