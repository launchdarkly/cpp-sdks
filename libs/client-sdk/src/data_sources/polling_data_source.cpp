#include <boost/json.hpp>

#include <launchdarkly/client_side/data_source_status.hpp>
#include <launchdarkly/config/shared/builders/http_properties_builder.hpp>
#include <launchdarkly/config/shared/sdks.hpp>
#include <launchdarkly/encoding/base_64.hpp>
#include <launchdarkly/network/http_error_messages.hpp>
#include <launchdarkly/serialization/json_context.hpp>

#include "data_source_update_sink.hpp"
#include "polling_data_source.hpp"

namespace launchdarkly::client_side::data_sources {

static char const* const kCouldNotParseEndpoint =
    "Could not parse polling endpoint URL.";

static network::HttpRequest MakeRequest(
    config::shared::built::ServiceEndpoints const& endpoints,
    config::shared::built::DataSourceConfig<config::shared::ClientSDK>
        data_source_config,
    config::shared::built::HttpProperties const& http_properties,
    Context const& context) {
    auto url = std::make_optional(endpoints.PollingBaseUrl());

    auto const& polling_config = std::get<
        config::shared::built::PollingConfig<config::shared::ClientSDK>>(
        data_source_config.method);

    auto string_context =
        boost::json::serialize(boost::json::value_from(context));

    network::HttpRequest::BodyType body;
    network::HttpMethod method = network::HttpMethod::kGet;

    if (data_source_config.use_report) {
        url = network::AppendUrl(url, polling_config.polling_report_path);
        method = network::HttpMethod::kReport;
        body = string_context;
    } else {
        url = network::AppendUrl(url, polling_config.polling_get_path);
        // When not using 'REPORT' we need to base64
        // encode the context so that we can safely
        // put it in a url.
        url =
            network::AppendUrl(url, encoding::Base64UrlEncode(string_context));
    }

    if (data_source_config.with_reasons) {
        if (url) {
            url->append("?withReasons=true");
        }
    }

    config::shared::builders::HttpPropertiesBuilder<config::shared::ClientSDK>
        builder(http_properties);

    // If no URL is set, then we will fail the request.
    return {url.value_or(""), method, builder.Build(), body};
}

PollingDataSource::PollingDataSource(
    config::shared::built::ServiceEndpoints const& endpoints,
    config::shared::built::DataSourceConfig<config::shared::ClientSDK> const&
        data_source_config,
    config::shared::built::HttpProperties const& http_properties,
    boost::asio::any_io_executor ioc,
    Context const& context,
    IDataSourceUpdateSink& handler,
    DataSourceStatusManager& status_manager,
    Logger const& logger)
    : ioc_(ioc),
      logger_(logger),
      status_manager_(status_manager),
      data_source_handler_(
          DataSourceEventHandler(context, handler, logger, status_manager_)),
      requester_(ioc),
      timer_(ioc),
      polling_interval_(
          std::get<
              config::shared::built::PollingConfig<config::shared::ClientSDK>>(
              data_source_config.method)
              .poll_interval),
      request_(MakeRequest(endpoints,
                           data_source_config,
                           http_properties,
                           context)) {
    auto const& polling_config = std::get<
        config::shared::built::PollingConfig<config::shared::ClientSDK>>(
        data_source_config.method);
    if (polling_interval_ < polling_config.min_polling_interval) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "Polling interval too frequent, defaulting to "
            << std::chrono::duration_cast<std::chrono::seconds>(
                   polling_config.min_polling_interval)
                   .count()
            << " seconds";

        polling_interval_ = polling_config.min_polling_interval;
    }
}

void PollingDataSource::DoPoll() {
    last_poll_start_ = std::chrono::system_clock::now();

    auto weak_self = weak_from_this();
    requester_.Request(request_, [weak_self](network::HttpResult res) {
        if (auto self = weak_self.lock()) {
            self->HandlePollResult(std::move(res));
        }
    });
}

void PollingDataSource::HandlePollResult(network::HttpResult res) {
    auto header_etag = res.Headers().find("etag");
    bool has_etag = header_etag != res.Headers().end();

    if (etag_ && has_etag) {
        if (etag_.value() == header_etag->second) {
            // Got the same etag, we know the content has not changed.
            // So we can just start the next timer.

            // We don't need to update the "request_" because it would have
            // the same Etag.
            StartPollingTimer();
            return;
        }
    }

    if (has_etag) {
        config::shared::builders::HttpPropertiesBuilder<
            config::shared::ClientSDK>
            builder(request_.Properties());
        builder.Header("If-None-Match", header_etag->second);
        request_ = network::HttpRequest(request_, builder.Build());

        etag_ = header_etag->second;
    }

    if (res.IsError()) {
        status_manager_.SetState(
            DataSourceStatus::DataSourceState::kInterrupted,
            DataSourceStatus::ErrorInfo::ErrorKind::kNetworkError,
            res.ErrorMessage() ? *res.ErrorMessage() : "unknown error");
        LD_LOG(logger_, LogLevel::kWarn)
            << "Polling for feature flag updates failed: "
            << (res.ErrorMessage() ? *res.ErrorMessage() : "unknown error");
    } else if (res.Status() == 200) {
        data_source_handler_.HandleMessage("put", res.Body().value());
    } else if (res.Status() == 304) {
        // This should be handled ahead of here, but if we get a 304,
        // and it didn't have an etag, we still don't want to try to
        // parse the body.
    } else {
        if (network::IsRecoverableStatus(res.Status())) {
            status_manager_.SetState(
                DataSourceStatus::DataSourceState::kInterrupted, res.Status(),
                launchdarkly::network::ErrorForStatusCode(
                    res.Status(), "polling request", "will retry"));
        } else {
            status_manager_.SetState(
                DataSourceStatus::DataSourceState::kShutdown, res.Status(),
                launchdarkly::network::ErrorForStatusCode(
                    res.Status(), "polling request", std::nullopt));
            // We are giving up. Do not start a new polling request.
            return;
        }
    }

    StartPollingTimer();
}

void PollingDataSource::StartPollingTimer() {
    auto time_since_poll_seconds =
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now() - last_poll_start_);

    // Calculate a delay based on the polling interval and the duration elapsed
    // since the last poll.

    // Example: If the poll took 5 seconds, and the interval is 30 seconds, then
    // we want to poll after 25 seconds. We do not want the interval to be
    // negative, so we clamp it to 0.
    auto delay = std::chrono::seconds(std::max(
        polling_interval_ - time_since_poll_seconds, std::chrono::seconds(0)));

    timer_.cancel();
    timer_.expires_after(delay);

    auto weak_self = weak_from_this();

    timer_.async_wait([weak_self](boost::system::error_code const& ec) {
        if (ec == boost::asio::error::operation_aborted) {
            // The timer was cancelled. Stop polling.
            return;
        }
        if (auto self = weak_self.lock()) {
            if (ec) {
                // Something unexpected happened. Log it and continue to try
                // polling.
                LD_LOG(self->logger_, LogLevel::kError)
                    << "Unexpected error in polling timer: " << ec.message();
            }
            self->DoPoll();
        }
    });
}

void PollingDataSource::Start() {
    status_manager_.SetState(DataSourceStatus::DataSourceState::kInitializing);
    if (!request_.Valid()) {
        LD_LOG(logger_, LogLevel::kError) << kCouldNotParseEndpoint;
        status_manager_.SetState(
            DataSourceStatus::DataSourceState::kShutdown,
            DataSourceStatus::ErrorInfo::ErrorKind::kNetworkError,
            kCouldNotParseEndpoint);

        // No need to attempt to poll if the URL is not valid.
        return;
    }

    DoPoll();
}

void PollingDataSource::ShutdownAsync(std::function<void()> completion) {
    status_manager_.SetState(DataSourceStatus::DataSourceState::kInitializing);
    timer_.cancel();
    if (completion) {
        boost::asio::post(timer_.get_executor(), completion);
    }
}

}  // namespace launchdarkly::client_side::data_sources
