#include "streaming_data_source.hpp"

#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/detail/unreachable.hpp>
#include <launchdarkly/encoding/base_64.hpp>
#include <launchdarkly/network/http_requester.hpp>
#include <launchdarkly/serialization/json_context.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/json.hpp>
#include <boost/url.hpp>

#include <utility>

namespace launchdarkly::client_side::data_sources {

static char const* const kCouldNotParseEndpoint =
    "Could not parse streaming endpoint URL.";

StreamingDataSource::StreamingDataSource(
    config::shared::built::ServiceEndpoints const& endpoints,
    config::shared::built::DataSourceConfig<config::shared::ClientSDK> const&
        data_source_config,
    config::shared::built::HttpProperties const& http_properties,
    boost::asio::any_io_executor ioc,
    Context context,
    IDataSourceUpdateSink& handler,
    DataSourceStatusManager& status_manager,
    Logger const& logger)
    : exec_(ioc),
      logger_(logger),
      context_(std::move(context)),
      status_manager_(status_manager),
      data_source_handler_(
          DataSourceEventHandler(context_, handler, logger, status_manager_)),
      http_config_(http_properties),
      data_source_config_(data_source_config),
      streaming_endpoint_(endpoints.StreamingBaseUrl()) {}

void StreamingDataSource::Start() {
    status_manager_.SetState(DataSourceStatus::DataSourceState::kInitializing);
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
        updated_url = network::AppendUrl(
            updated_url, encoding::Base64UrlEncode(string_context));
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

    client_builder.initial_reconnect_delay(
        streaming_config.initial_reconnect_delay);

    for (auto const& header : http_config_.BaseHeaders()) {
        client_builder.header(header.first, header.second);
    }

    if (http_config_.Tls().PeerVerifyMode() ==
        config::shared::built::TlsOptions::VerifyMode::kVerifyNone) {
        client_builder.skip_verify_peer(true);
    }

    if (auto ca_file = http_config_.Tls().CustomCAFile()) {
        client_builder.custom_ca_file(*ca_file);
    }

    if (http_config_.HttpProxy()) {
        client_builder.http_proxy(*http_config_.HttpProxy());
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
            LD_LOG(self->logger_, LogLevel::kDebug) << "sse-client: " << msg;
        }
    });

    client_builder.errors([weak_self](auto error) {
        if (auto self = weak_self.lock()) {
            std::string error_string = sse::ErrorToString(error);
            LD_LOG(self->logger_, sse::IsRecoverable(error) ? LogLevel::kDebug
                                                            : LogLevel::kError);
            self->HandleErrorStateChange(std::move(error),
                                         std::move(error_string));
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
    client_->async_connect();
}

template <class>
inline constexpr bool always_false_v = false;

void StreamingDataSource::HandleErrorStateChange(sse::Error error,
                                                 std::string error_string) {
    auto const state = sse::IsRecoverable(error) ? DataSourceState::kInterrupted
                                                 : DataSourceState::kShutdown;
    std::visit(
        [this, state, error_string = std::move(error_string)](auto error) {
            using T = std::decay_t<decltype(error)>;
            if constexpr (std::is_same_v<T, sse::errors::ReadTimeout>) {
                this->status_manager_.SetState(
                    state,
                    DataSourceStatus::ErrorInfo::ErrorKind::kNetworkError,
                    std::move(error_string));

            } else if constexpr (std::is_same_v<
                                     T,
                                     sse::errors::UnrecoverableClientError>) {
                this->status_manager_.SetState(
                    state,
                    static_cast<DataSourceStatusManager::StatusCodeType>(
                        error.status),
                    std::move(error_string));

            } else if constexpr (std::is_same_v<
                                     T, sse::errors::InvalidRedirectLocation>) {
                this->status_manager_.SetState(
                    state,
                    DataSourceStatus::ErrorInfo::ErrorKind::kNetworkError,
                    std::move(error_string));

            } else if constexpr (std::is_same_v<T,
                                                sse::errors::NotRedirectable>) {
                this->status_manager_.SetState(
                    state,
                    DataSourceStatus::ErrorInfo::ErrorKind::kNetworkError,
                    std::move(error_string));
            } else {
                static_assert(always_false_v<decltype(error)>,
                              "non-exhaustive visitor");
            }
        },
        std::move(error));
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

}  // namespace launchdarkly::client_side::data_sources
