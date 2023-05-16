#pragma once

#include <chrono>

#include <boost/asio/any_io_executor.hpp>

#include <launchdarkly/client_side/data_source.hpp>
#include <launchdarkly/client_side/data_source_update_sink.hpp>
#include <launchdarkly/client_side/data_sources/detail/data_source_event_handler.hpp>
#include <launchdarkly/config/client.hpp>
#include <launchdarkly/config/detail/built/http_properties.hpp>
#include <launchdarkly/logger.hpp>
#include <launchdarkly/network/detail/asio_requester.hpp>
#include "data_source_status_manager.hpp"

namespace launchdarkly::client_side::data_sources::detail {

class PollingDataSource
    : public IDataSource,
      public std::enable_shared_from_this<PollingDataSource> {
   public:
    PollingDataSource(Config const& config,
                      boost::asio::any_io_executor ioc,
                      Context const& context,
                      IDataSourceUpdateSink* handler,
                      DataSourceStatusManager& status_manager,
                      Logger const& logger);

    void Start() override;
    void Close();

    void AsyncShutdown(std::function<void()>) override;

   private:
    void DoPoll();
    void HandlePollResult(network::detail::HttpResult res);

    std::string string_context_;
    DataSourceStatusManager& status_manager_;
    DataSourceEventHandler data_source_handler_;
    std::string polling_endpoint_;

    network::detail::AsioRequester requester_;
    Logger const& logger_;
    boost::asio::any_io_executor ioc_;
    std::chrono::seconds polling_interval_;
    network::detail::HttpRequest request_;
    std::optional<std::string> etag_;

    boost::asio::steady_timer timer_;
    std::chrono::time_point<std::chrono::system_clock> last_poll_start_;

    void StartPollingTimer();
};

}  // namespace launchdarkly::client_side::data_sources::detail
