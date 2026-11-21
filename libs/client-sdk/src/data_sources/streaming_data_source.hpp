#pragma once

#include <boost/asio/any_io_executor.hpp>

#include "data_source.hpp"
#include "data_source_event_handler.hpp"
#include "data_source_status_manager.hpp"
#include "data_source_update_sink.hpp"
#include "launchdarkly/config/shared/sdks.hpp"

#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_result.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/network/requester.hpp>
#include <launchdarkly/sse/client.hpp>

#include <atomic>
#include <chrono>

namespace launchdarkly::client_side::data_sources {

/**
 * Streams the flags of one context. When the stream sends a "ping" event, the
 * data source polls for the flags of the context and applies the result.
 * Call Start at most once. After ShutdownAsync, the data source does not
 * start again.
 *
 * Thread safety: Start and ShutdownAsync are safe from any thread, but not at
 * the same time. The stream, poll and error callbacks run on the executor
 * given to the constructor. That executor must run on one thread, because the
 * data source relies on it to run those callbacks one at a time.
 */
class StreamingDataSource final
    : public IDataSource,
      public std::enable_shared_from_this<StreamingDataSource> {
   public:
    StreamingDataSource(
        config::shared::built::ServiceEndpoints const& endpoints,
        config::shared::built::DataSourceConfig<
            config::shared::ClientSDK> const& data_source_config,
        config::shared::built::HttpProperties const& http_properties,
        boost::asio::any_io_executor ioc,
        Context context,
        IDataSourceUpdateSink& handler,
        DataSourceStatusManager& status_manager,
        Logger const& logger);

    void Start() override;
    void ShutdownAsync(std::function<void()>) override;

   private:
    void HandleErrorStateChange(sse::Error error, std::string error_string);
    void PollAfterPing();
    void HandlePingPollResult(network::HttpResult const& res);

    Context context_;
    boost::asio::any_io_executor exec_;
    DataSourceStatusManager& status_manager_;
    DataSourceEventHandler data_source_handler_;
    std::string streaming_endpoint_;
    std::string string_context_;

    config::shared::built::DataSourceConfig<config::shared::ClientSDK>
        data_source_config_;

    config::shared::built::HttpProperties http_config_;

    Logger const& logger_;
    std::shared_ptr<launchdarkly::sse::Client> client_;
    network::Requester requester_;
    network::HttpRequest ping_request_;

    // Used only on the executor thread.
    bool ping_poll_in_flight_ = false;
    bool ping_poll_pending_ = false;

    // ShutdownAsync and an unrecoverable stream error set this. ShutdownAsync
    // can run on a different thread.
    std::atomic<bool> stopped_{false};
};
}  // namespace launchdarkly::client_side::data_sources
