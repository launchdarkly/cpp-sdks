#pragma once

#include <chrono>
using namespace std::chrono_literals;

#include <boost/asio/any_io_executor.hpp>

#include "config/detail/service_endpoints.hpp"
#include "context.hpp"
#include "data/evaluation_result.hpp"
#include "launchdarkly/data_source.hpp"
#include "launchdarkly/data_source_update_sink.hpp"
#include "launchdarkly/sse/client.hpp"
#include "logger.hpp"

namespace launchdarkly::client_side {

struct HttpProperties {
    std::chrono::duration<int, std::milli> connect_timeout;
    std::chrono::duration<int, std::milli> read_timeout;
    std::string user_agent;
    std::map<std::string, std::vector<std::string>> base_headers;
    // Proxy?
};

class StreamingDataSource final : public IDataSource {
   public:
    // We may want to replace this with a builder?
    StreamingDataSource(
        boost::asio::any_io_executor event,
        Context const& context,
        bool use_report,
        bool with_reasons,
        std::chrono::duration<int, std::milli> initial_retry_delay,
        config::ServiceEndpoints const& endpoints,
        HttpProperties const& http_properties,
        std::shared_ptr<IDataSourceUpdateSink> handler,
        Logger const& logger);

    void start() override;
    void close() override;

    struct PatchData {
        std::string key;
        EvaluationResult flag;
    };

    struct DeleteData {
        std::string key;
        uint64_t version;
    };

   private:
    void handle_message(launchdarkly::sse::Event const& event);

    std::shared_ptr<IDataSourceUpdateSink> handler_;
    std::string streaming_endpoint_;
    std::string string_context_;
    HttpProperties http_properties_;
    Logger const& logger_;

    std::shared_ptr<launchdarkly::sse::Client> client_;
    boost::asio::any_io_executor executor_;

    inline static const std::string streaming_path_ = "/meval";
};
}  // namespace launchdarkly::client_side
