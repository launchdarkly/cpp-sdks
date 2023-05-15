#pragma once

#include <chrono>
using namespace std::chrono_literals;

#include <boost/asio/any_io_executor.hpp>

#include <launchdarkly/client_side/data_source.hpp>
#include <launchdarkly/client_side/data_source_update_sink.hpp>
#include <launchdarkly/client_side/data_sources/detail/data_source_event_handler.hpp>
#include <launchdarkly/client_side/data_sources/detail/data_source_status_manager.hpp>
#include <launchdarkly/config/client.hpp>
#include <launchdarkly/config/detail/built/http_properties.hpp>
#include <launchdarkly/config/detail/built/service_endpoints.hpp>
#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_result.hpp>
#include <launchdarkly/logger.hpp>
#include <launchdarkly/sse/client.hpp>

namespace launchdarkly::client_side::data_sources::detail {

class StreamingDataSource final
    : public IDataSource,
      public std::enable_shared_from_this<StreamingDataSource> {
   public:
    StreamingDataSource(Config const& config,
                        boost::asio::any_io_executor ioc,
                        Context context,
                        IDataSourceUpdateSink* handler,
                        DataSourceStatusManager& status_manager,
                        Logger const& logger);

    void Start() override;
    void AsyncShutdown(std::function<void()>) override;
    std::future<void> SyncShutdown() override;

    ~StreamingDataSource();

   private:
    boost::asio::any_io_executor exec_;
    DataSourceStatusManager& status_manager_;
    DataSourceEventHandler data_source_handler_;
    std::string streaming_endpoint_;
    std::string string_context_;

    Context context_;

    config::detail::built::DataSourceConfig<config::detail::ClientSDK>
        data_source_config_;

    config::detail::built::HttpProperties http_config_;

    std::string sdk_key_;

    Logger const& logger_;
    std::shared_ptr<launchdarkly::sse::Client> client_;
};
}  // namespace launchdarkly::client_side::data_sources::detail
