#pragma once

#include <chrono>
using namespace std::chrono_literals;

#include <boost/asio/any_io_executor.hpp>

#include "config/client.hpp"
#include "config/detail/built/http_properties.hpp"
#include "config/detail/built/service_endpoints.hpp"
#include "context.hpp"
#include "data/evaluation_result.hpp"
#include "launchdarkly/client_side/data_source.hpp"
#include "launchdarkly/client_side/data_source_update_sink.hpp"
#include "launchdarkly/client_side/data_sources/detail/data_source_event_handler.hpp"
#include "launchdarkly/client_side/data_sources/detail/data_source_status_manager.hpp"
#include "launchdarkly/sse/client.hpp"
#include "logger.hpp"

namespace launchdarkly::client_side::data_sources::detail {

class StreamingDataSource final : public IDataSource {
   public:
    StreamingDataSource(Config const& config,
                        boost::asio::any_io_executor ioc,
                        Context const& context,
                        IDataSourceUpdateSink* handler,
                        DataSourceStatusManager& status_manager,
                        Logger const& logger);

    void Start() override;
    void AsyncShutdown(std::function<void()>) override;
    std::future<void> SyncShutdown() override;
    
    ~StreamingDataSource();

   private:
    DataSourceStatusManager& status_manager_;
    DataSourceEventHandler data_source_handler_;
    std::string streaming_endpoint_;
    std::string string_context_;

    Logger const& logger_;
    std::shared_ptr<launchdarkly::sse::Client> client_;
};
}  // namespace launchdarkly::client_side::data_sources::detail
