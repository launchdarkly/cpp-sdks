#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/url.hpp>

#include <utility>

#include "streaming_data_source.hpp"

#include <launchdarkly/network/http_requester.hpp>

namespace launchdarkly::server_side::data {

static char const* const kCouldNotParseEndpoint =
    "Could not parse streaming endpoint URL";

static char const* DataSourceErrorToString(launchdarkly::sse::Error error) {
    switch (error) {
        case sse::Error::NoContent:
            return "server responded 204 (No Content), will not attempt to "
                   "reconnect";
        case sse::Error::InvalidRedirectLocation:
            return "server responded with an invalid redirection";
        case sse::Error::UnrecoverableClientError:
            return "unrecoverable client-side error";
        default:
            return "unrecognized error";
    }
}

StreamingDataSource::StreamingDataSource(
    config::shared::built::ServiceEndpoints const& endpoints,
    config::shared::built::StreamingConfig<config::shared::ServerSDK> const&
        data_source_config,
    config::shared::built::HttpProperties http_properties,
    boost::asio::any_io_executor ioc,
    IDataSourceUpdateSink& handler,
    DataSourceStatusManager& status_manager,
    Logger const& logger)
    : exec_(std::move(ioc)),
      logger_(logger),
      status_manager_(status_manager),
      data_source_handler_(
          DataSourceEventHandler(handler, logger, status_manager_)),
      http_config_(std::move(http_properties)),
      streaming_config_(data_source_config),
      streaming_endpoint_(endpoints.StreamingBaseUrl()) {}

void StreamingDataSource::Init(
    std::optional<data_model::SDKDataSet> initial_data,
    IDataDestination& destination) {
    // TODO: implement
}

void StreamingDataSource::Start() {
    status_manager_.SetState(DataSourceStatus::DataSourceState::kInitializing);

    auto updated_url = network::AppendUrl(streaming_endpoint_,
                                          streaming_config_.streaming_path);

    // Bad URL, don't set the client. Start will then report the bad status.
    if (!updated_url) {
        LD_LOG(logger_, LogLevel::kError) << kCouldNotParseEndpoint;
        status_manager_.SetState(
            DataSourceStatus::DataSourceState::kOff,
            DataSourceStatus::ErrorInfo::ErrorKind::kNetworkError,
            kCouldNotParseEndpoint);
        return;
    }

    auto uri_components = boost::urls::parse_uri(*updated_url);

    // Unlikely that it could be parsed earlier, and it cannot be parsed now.
    if (!uri_components) {
        LD_LOG(logger_, LogLevel::kError) << kCouldNotParseEndpoint;
        status_manager_.SetState(
            DataSourceStatus::DataSourceState::kOff,
            DataSourceStatus::ErrorInfo::ErrorKind::kNetworkError,
            kCouldNotParseEndpoint);
        return;
    }

    boost::urls::url url = uri_components.value();

    auto client_builder = launchdarkly::sse::Builder(exec_, url.buffer());

    client_builder.method(boost::beast::http::verb::get);

    // TODO: can the read timeout be shared with *all* http requests? Or should
    // it have a default in defaults.hpp? This must be greater than the
    // heartbeat interval of the streaming service.
    client_builder.read_timeout(std::chrono::minutes(5));

    client_builder.write_timeout(http_config_.WriteTimeout());

    client_builder.connect_timeout(http_config_.ConnectTimeout());

    client_builder.initial_reconnect_delay(
        streaming_config_.initial_reconnect_delay);

    for (auto const& header : http_config_.BaseHeaders()) {
        client_builder.header(header.first, header.second);
    }

    auto weak_self = weak_from_this();

    client_builder.receiver([weak_self](launchdarkly::sse::Event const& event) {
        if (auto self = weak_self.lock()) {
            self->data_source_handler_.HandleMessage(event.type(),
                                                     event.data());
            // TODO: Use the result of handle message to restart the
            // event source if we got bad data. sc-204387
        }
    });

    client_builder.logger([weak_self](auto msg) {
        if (auto self = weak_self.lock()) {
            LD_LOG(self->logger_, LogLevel::kDebug) << msg;
        }
    });

    client_builder.errors([weak_self](auto error) {
        if (auto self = weak_self.lock()) {
            auto error_string = DataSourceErrorToString(error);
            LD_LOG(self->logger_, LogLevel::kError) << error_string;
            self->status_manager_.SetState(
                DataSourceStatus::DataSourceState::kOff,
                DataSourceStatus::ErrorInfo::ErrorKind::kErrorResponse,
                error_string);
        }
    });

    client_ = client_builder.build();

    if (!client_) {
        LD_LOG(logger_, LogLevel::kError) << kCouldNotParseEndpoint;
        status_manager_.SetState(
            DataSourceStatus::DataSourceState::kOff,
            DataSourceStatus::ErrorInfo::ErrorKind::kNetworkError,
            kCouldNotParseEndpoint);
        return;
    }
    client_->async_connect();
}

void StreamingDataSource::ShutdownAsync(std::function<void()> completion) {
    if (client_) {
        status_manager_.SetState(
            DataSourceStatus::DataSourceState::kInitializing);
        return client_->async_shutdown(std::move(completion));
    }
    if (completion) {
        boost::asio::post(exec_, completion);
    }
}
}  // namespace launchdarkly::server_side::data
