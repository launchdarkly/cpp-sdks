#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/json.hpp>
#include <boost/url.hpp>

#include <utility>

#include "config/detail/defaults.hpp"
#include "context.hpp"
#include "context_builder.hpp"
#include "launchdarkly/client_side/data_sources/detail/base_64.hpp"
#include "launchdarkly/client_side/data_sources/detail/streaming_data_source.hpp"
#include "network/detail/http_requester.hpp"
#include "serialization/json_context.hpp"

namespace launchdarkly::client_side::data_sources::detail {

static char const* const kCouldNotParseEndpoint =
    "Could not parse streaming endpoint URL.";

StreamingDataSource::StreamingDataSource(
    Config const& config,
    boost::asio::any_io_executor ioc,
    Context const& context,
    IDataSourceUpdateSink* handler,
    DataSourceStatusManager& status_manager,
    Logger const& logger)
    : logger_(logger),
      status_manager_(status_manager),
      data_source_handler_(
          DataSourceEventHandler(handler, logger, status_manager_)) {
    auto string_context =
        boost::json::serialize(boost::json::value_from(context));

    auto& data_source_config = config.DataSourceConfig();

    auto& streaming_config = boost::get<
        config::detail::built::StreamingConfig<config::detail::ClientSDK>>(
        data_source_config.method);

    auto updated_url =
        network::detail::AppendUrl(config.ServiceEndpoints().StreamingBaseUrl(),
                                   streaming_config.streaming_path);

    if (!data_source_config.use_report) {
        // When not using 'REPORT' we need to base64
        // encode the context so that we can safely
        // put it in a url.
        updated_url = network::detail::AppendUrl(
            updated_url, Base64UrlEncode(string_context));
    }
    // Bad URL, don't set the client. Start will then report the bad status.
    if (!updated_url) {
        return;
    }

    auto uri_components = boost::urls::parse_uri(*updated_url);

    // Unlikely that it could be parsed earlier and it cannot be parsed now.
    if (!uri_components) {
        return;
    }

    // TODO: Initial reconnect delay.
    boost::urls::url url = uri_components.value();

    if (data_source_config.with_reasons) {
        url.params().set("withReasons", "true");
    }

    auto client_builder =
        launchdarkly::sse::Builder(std::move(ioc), url.buffer());

    client_builder.method(data_source_config.use_report
                              ? boost::beast::http::verb::report
                              : boost::beast::http::verb::get);

    client_builder.receiver([this](launchdarkly::sse::Event const& event) {
        data_source_handler_.HandleMessage(event.type(), event.data());
        // TODO: Use the result of handle message to restart the
        // event source if we got bad data.
    });

    client_builder.logger(
        [this](auto msg) { LD_LOG((logger_), LogLevel::kInfo) << msg; });

    if (data_source_config.use_report) {
        client_builder.body(string_context);
    }

    auto& http_properties = config.HttpProperties();

    client_builder.header("authorization", config.SdkKey());
    for (auto const& header : http_properties.BaseHeaders()) {
        client_builder.header(header.first, header.second);
    }
    client_builder.header("user-agent", http_properties.UserAgent());
    // TODO: Handle proxy support.
    client_ = client_builder.build();
}

void StreamingDataSource::Start() {
    if (!client_) {
        LD_LOG(logger_, LogLevel::kError) << kCouldNotParseEndpoint;
        status_manager_.SetState(
            DataSourceStatus::DataSourceState::kShutdown,
            DataSourceStatus::ErrorInfo::ErrorKind::kNetworkError,
            kCouldNotParseEndpoint);
        return;
    }
    client_->run();
}

void StreamingDataSource::Close() {
    status_manager_.SetState(DataSourceStatus::DataSourceState::kShutdown);
    if (client_) {
        client_->close();
    }
}

}  // namespace launchdarkly::client_side::data_sources::detail
