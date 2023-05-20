#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/json.hpp>
#include <boost/url.hpp>

#include <utility>

#include "base_64.hpp"
#include "streaming_data_source.hpp"

#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/network/http_requester.hpp>
#include <launchdarkly/serialization/json_context.hpp>

namespace launchdarkly::client_side::data_sources {

static char const* const kCouldNotParseEndpoint =
    "Could not parse streaming endpoint URL.";

StreamingDataSource::StreamingDataSource(
    Config const& config,
    boost::asio::any_io_executor ioc,
    Context context,
    IDataSourceUpdateSink* handler,
    DataSourceStatusManager& status_manager,
    Logger const& logger)
    : exec_(ioc),
      logger_(logger),
      status_manager_(status_manager),
      data_source_handler_(
          DataSourceEventHandler(handler, logger, status_manager_)),
      context_(std::move(context)),
      http_config_(config.HttpProperties()),
      data_source_config_(config.DataSourceConfig()),
      app_tags_(config.ApplicationTag()),
      sdk_key_(config.SdkKey()),
      streaming_endpoint_(config.ServiceEndpoints().StreamingBaseUrl()) {}

void StreamingDataSource::Start() {
    auto string_context =
        boost::json::serialize(boost::json::value_from(context_));

    auto const& streaming_config = std::get<
        config::shared::built::StreamingConfig<config::shared::ClientSDK>>(
        data_source_config_.method);

    auto updated_url = network::AppendUrl(streaming_endpoint_,
                                          streaming_config.streaming_path);

    if (!data_source_config_.use_report) {
        // When not using 'REPORT' we need to base64
        // encode the context so that we can safely
        // put it in a url.
        updated_url =
            network::AppendUrl(updated_url, Base64UrlEncode(string_context));
    }
    // Bad URL, don't set the client. Start will then report the bad status.
    if (!updated_url) {
        LD_LOG(logger_, LogLevel::kError) << kCouldNotParseEndpoint;
        status_manager_.SetState(
            DataSourceStatus::DataSourceState::kShutdown,
            DataSourceStatus::ErrorInfo::ErrorKind::kNetworkError,
            kCouldNotParseEndpoint);
        return;
    }

    auto uri_components = boost::urls::parse_uri(*updated_url);

    // Unlikely that it could be parsed earlier and it cannot be parsed now.
    if (!uri_components) {
        LD_LOG(logger_, LogLevel::kError) << kCouldNotParseEndpoint;
        status_manager_.SetState(
            DataSourceStatus::DataSourceState::kShutdown,
            DataSourceStatus::ErrorInfo::ErrorKind::kNetworkError,
            kCouldNotParseEndpoint);
        return;
    }

    // TODO: Initial reconnect delay.
    boost::urls::url url = uri_components.value();

    if (data_source_config_.with_reasons) {
        url.params().set("withReasons", "true");
    }

    auto client_builder = launchdarkly::sse::Builder(exec_, url.buffer());

    client_builder.method(data_source_config_.use_report
                              ? boost::beast::http::verb::report
                              : boost::beast::http::verb::get);

    if (data_source_config_.use_report) {
        client_builder.body(string_context);
    }

    // TODO: can the read timeout be shared with *all* http requests? Or should
    // it have a default in defaults.hpp? This must be greater than the
    // heartbeat interval of the streaming service.
    client_builder.read_timeout(std::chrono::minutes(5));

    client_builder.write_timeout(http_config_.WriteTimeout());

    client_builder.connect_timeout(http_config_.ConnectTimeout());

    client_builder.header("authorization", sdk_key_);
    for (auto const& header : http_config_.BaseHeaders()) {
        client_builder.header(header.first, header.second);
    }
    client_builder.header("user-agent", http_config_.UserAgent());

    if (app_tags_) {
        client_builder.header("x-launchdarkly-tags", *app_tags_);
    }

    // TODO: Handle proxy support.

    auto weak_self = weak_from_this();

    client_builder.receiver([weak_self](launchdarkly::sse::Event const& event) {
        if (auto self = weak_self.lock()) {
            self->data_source_handler_.HandleMessage(event.type(),
                                                     event.data());
            // TODO: Use the result of handle message to restart the
            // event source if we got bad data.
        }
    });

    client_builder.logger([weak_self](auto msg) {
        if (auto self = weak_self.lock()) {
            LD_LOG(self->logger_, LogLevel::kInfo) << msg;
        }
    });

    client_ = client_builder.build();

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

void StreamingDataSource::ShutdownAsync(std::function<void()> completion) {
    if (client_) {
        status_manager_.SetState(DataSourceStatus::DataSourceState::kSlept);
        return client_->async_shutdown(std::move(completion));
    }
    if (completion) {
        boost::asio::post(exec_, completion);
    }
}

}  // namespace launchdarkly::client_side::data_sources
