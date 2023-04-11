#pragma once

#include <chrono>
using namespace std::chrono_literals;

#include <boost/asio/any_io_executor.hpp>

#include "config/detail/service_endpoints.hpp"
#include "context.hpp"
#include "data/evaluation_result.hpp"
#include "launchdarkly/client_side/data_source.hpp"
#include "launchdarkly/client_side/data_source_update_sink.hpp"
#include "launchdarkly/client_side/data_sources/detail/streaming_data_handler.hpp"
#include "launchdarkly/sse/client.hpp"
#include "logger.hpp"

namespace launchdarkly::client_side::data_sources::detail {

class StreamingDataSource final : public IDataSource {
   public:
    StreamingDataSource(std::string sdk_key,
                        boost::asio::any_io_executor ioc,
                        Context const& context,
                        config::ServiceEndpoints const& endpoints,
                        config::detail::HttpProperties const& http_properties,
                        bool use_report,
                        bool with_reasons,
                        std::shared_ptr<IDataSourceUpdateSink> handler,
                        Logger const& logger);

    void start() override;
    void close() override;

   private:
    StreamingDataHandler data_source_handler_;
    std::string streaming_endpoint_;
    std::string string_context_;

    Logger const& logger_;
    std::shared_ptr<launchdarkly::sse::Client> client_;

    inline static const std::string streaming_path_ = "/meval";
};
}  // namespace launchdarkly::client_side::data_sources::detail
