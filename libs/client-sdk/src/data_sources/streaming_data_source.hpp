#pragma once

#include <chrono>
using namespace std::chrono_literals;

#include <boost/asio/any_io_executor.hpp>

#include "data_source.hpp"
#include "data_source_event_handler.hpp"
#include "data_source_status_manager.hpp"
#include "data_source_update_sink.hpp"

#include <launchdarkly/config/client.hpp>
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_result.hpp>
#include <launchdarkly/sse/client.hpp>
#include "launchdarkly/logging/logger.hpp"

namespace launchdarkly::client_side::data_sources {

class StreamingDataSource final : public IDataSource {
   public:
    StreamingDataSource(Config const& config,
                        boost::asio::any_io_executor ioc,
                        Context const& context,
                        IDataSourceUpdateSink* handler,
                        DataSourceStatusManager& status_manager,
                        Logger const& logger);

    void Start() override;
    void Close() override;

   private:
    DataSourceStatusManager& status_manager_;
    DataSourceEventHandler data_source_handler_;
    std::string streaming_endpoint_;
    std::string string_context_;

    Logger const& logger_;
    std::shared_ptr<launchdarkly::sse::Client> client_;
};
}  // namespace launchdarkly::client_side::data_sources
