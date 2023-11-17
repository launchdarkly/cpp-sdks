#pragma once

#include "event_handler.hpp"

#include "../../../../data_components/status_notifications/data_source_status_manager.hpp"
#include "../../../../data_interfaces/destination/idestination.hpp"
#include "../../../../data_interfaces/source/ipush_source.hpp"

#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/config/shared/sdks.hpp>
#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_result.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/sse/client.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <chrono>

using namespace std::chrono_literals;

namespace launchdarkly::server_side::data_systems {

class StreamingDataSource final
    : public data_interfaces::IPushSource,
      public std::enable_shared_from_this<StreamingDataSource> {
   public:
    StreamingDataSource(
        ::launchdarkly::config::shared::built::ServiceEndpoints const&
            endpoints,
        ::launchdarkly::config::shared::built::StreamingConfig<
            ::launchdarkly::config::shared::ServerSDK> const&
            data_source_config,
        launchdarkly::config::shared::built::HttpProperties http_properties,
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

    ::launchdarkly::config::shared::built::StreamingConfig<
        ::launchdarkly::config::shared::ServerSDK>
        streaming_config_;

    ::launchdarkly::config::shared::built::HttpProperties http_config_;

    Logger const& logger_;
    std::shared_ptr<launchdarkly::sse::Client> client_;
};
}  // namespace launchdarkly::server_side::data_systems
