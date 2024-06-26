#include "streaming_data_source.hpp"

#include <launchdarkly/network/http_requester.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/url.hpp>

#include <utility>

namespace launchdarkly::server_side::data_systems {

static char const* const kCouldNotParseEndpoint =
    "Could not parse streaming endpoint URL";

std::string const& StreamingDataSource::Identity() const {
    static std::string const identity = "streaming data source";
    return identity;
}

StreamingDataSource::StreamingDataSource(
    boost::asio::any_io_executor io,
    Logger const& logger,
    data_components::DataSourceStatusManager& status_manager,
    config::built::ServiceEndpoints const& endpoints,
    config::built::BackgroundSyncConfig::StreamingConfig const& streaming,
    config::built::HttpProperties const& http_properties)
    : io_(std::move(io)),
      logger_(logger),
      status_manager_(status_manager),
      http_config_(http_properties),
      streaming_config_(streaming),
      streaming_endpoint_(endpoints.StreamingBaseUrl()) {}

void StreamingDataSource::StartAsync(
    data_interfaces::IDestination* dest,
    data_model::SDKDataSet const* bootstrap_data) {
    boost::ignore_unused(bootstrap_data);

    event_handler_.emplace(*dest, logger_, status_manager_);

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

    auto client_builder = launchdarkly::sse::Builder(io_, url.buffer());

    client_builder.method(boost::beast::http::verb::get);

    // TODO: can the read timeout be shared with *all* http requests? Or should
    // it have a default in defaults.hpp? This must be greater than the
    // heartbeat interval of the streaming service.
    client_builder.read_timeout(std::chrono::minutes(5));

    client_builder.write_timeout(http_config_.WriteTimeout());

    client_builder.connect_timeout(http_config_.ConnectTimeout());

    client_builder.initial_reconnect_delay(
        streaming_config_.initial_reconnect_delay);

    for (auto const& [key, value] : http_config_.BaseHeaders()) {
        client_builder.header(key, value);
    }

    if (http_config_.Tls().PeerVerifyMode() ==
        launchdarkly::config::shared::built::TlsOptions::VerifyMode::
            kVerifyNone) {
        client_builder.skip_verify_peer(true);
    }

    if (auto ca_file = http_config_.Tls().CustomCAFile()) {
        client_builder.custom_ca_file(*ca_file);
    }

    auto weak_self = weak_from_this();

    client_builder.receiver([weak_self](launchdarkly::sse::Event const& event) {
        if (auto self = weak_self.lock()) {
            self->event_handler_->HandleMessage(event.type(), event.data());
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
            std::string error_string = launchdarkly::sse::ErrorToString(error);
            LD_LOG(self->logger_, LogLevel::kError) << error_string;
            self->status_manager_.SetState(
                DataSourceStatus::DataSourceState::kOff,
                DataSourceStatus::ErrorInfo::ErrorKind::kErrorResponse,
                std::move(error_string));
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
        boost::asio::post(io_, completion);
    }
}
}  // namespace launchdarkly::server_side::data_systems
