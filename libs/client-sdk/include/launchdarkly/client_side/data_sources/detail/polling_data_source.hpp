#pragma once

#include <chrono>

#include <boost/asio/any_io_executor.hpp>

#include "config/detail/built/http_properties.hpp"
#include "data_source_status_manager.hpp"
#include "launchdarkly/client_side/data_source.hpp"
#include "launchdarkly/client_side/data_source_update_sink.hpp"
#include "launchdarkly/client_side/data_sources/detail/data_source_event_handler.hpp"
#include "logger.hpp"
#include "network/detail/asio_requester.hpp"

namespace launchdarkly::client_side::data_sources::detail {

const static std::chrono::seconds kMinPollingInterval =
    std::chrono::seconds{30};

class PollingDataSource : public IDataSource {
   public:
    PollingDataSource(
        std::string const& sdk_key,
        boost::asio::any_io_executor ioc,
        Context const& context,
        config::detail::built::ServiceEndpoints const& endpoints,
        config::detail::built::HttpProperties const& http_properties,
        std::chrono::seconds polling_interval,
        bool use_report,
        bool with_reasons,
        IDataSourceUpdateSink* handler,
        DataSourceStatusManager& status_manager,
        Logger const& logger);

    void Start() override;
    void Close() override;

   private:
    void DoPoll();

    std::string string_context_;
    DataSourceStatusManager& status_manager_;
    DataSourceEventHandler data_source_handler_;
    std::string polling_endpoint_;

    network::detail::AsioRequester requester_;
    Logger const& logger_;
    boost::asio::any_io_executor ioc_;
    std::chrono::seconds polling_interval_;
    network::detail::HttpRequest request_;

    // TODO: Unique/Shared ptr.
    boost::asio::deadline_timer* timer_;

    inline const static std::string polling_get_path_ = "/msdk/evalx/contexts";

    inline const static std::string polling_report_path_ =
        "/msdk/evalx/context";
};

}  // namespace launchdarkly::client_side::data_sources::detail