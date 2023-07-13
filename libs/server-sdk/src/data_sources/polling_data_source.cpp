#include <boost/json.hpp>

#include <launchdarkly/config/shared/builders/http_properties_builder.hpp>
#include <launchdarkly/config/shared/sdks.hpp>
#include <launchdarkly/encoding/base_64.hpp>
#include <launchdarkly/network/http_error_messages.hpp>
#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_primitives.hpp>
#include <launchdarkly/serialization/json_rule_clause.hpp>
#include <launchdarkly/serialization/json_sdk_data_set.hpp>
#include <launchdarkly/server_side/data_source_status.hpp>

#include "data_source_update_sink.hpp"
#include "polling_data_source.hpp"

namespace launchdarkly::server_side::data_sources {

static char const* const kErrorParsingPut = "Could not parse polling payload";
static char const* const kErrorPutInvalid =
    "polling payload contained invalid data";

static char const* const kCouldNotParseEndpoint =
    "Could not parse polling endpoint URL.";

static network::HttpRequest MakeRequest(
    config::shared::built::DataSourceConfig<config::shared::ServerSDK> const&
        data_source_config,
    config::shared::built::ServiceEndpoints const& endpoints,
    config::shared::built::HttpProperties const& http_properties) {
    auto url = std::make_optional(endpoints.PollingBaseUrl());

    auto const& polling_config = std::get<
        config::shared::built::PollingConfig<config::shared::ServerSDK>>(
        data_source_config.method);

    url = network::AppendUrl(url, polling_config.polling_get_path);

    network::HttpRequest::BodyType body;
    network::HttpMethod method = network::HttpMethod::kGet;

    config::shared::builders::HttpPropertiesBuilder<config::shared::ServerSDK>
        builder(http_properties);

    // If no URL is set, then we will fail the request.
    return {url.value_or(""), method, builder.Build(), body};
}

PollingDataSource::PollingDataSource(
    config::shared::built::ServiceEndpoints const& endpoints,
    config::shared::built::DataSourceConfig<config::shared::ServerSDK> const&
        data_source_config,
    config::shared::built::HttpProperties const& http_properties,
    boost::asio::any_io_executor const& ioc,
    IDataSourceUpdateSink& handler,
    DataSourceStatusManager& status_manager,
    Logger const& logger)
    : ioc_(ioc),
      logger_(logger),
      status_manager_(status_manager),
      update_sink_(handler),
      requester_(ioc),
      timer_(ioc),
      polling_interval_(
          std::get<
              config::shared::built::PollingConfig<config::shared::ServerSDK>>(
              data_source_config.method)
              .poll_interval),
      request_(MakeRequest(data_source_config, endpoints, http_properties)) {
    auto const& polling_config = std::get<
        config::shared::built::PollingConfig<config::shared::ServerSDK>>(
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
    requester_.Request(request_, [weak_self](network::HttpResult const& res) {
        if (auto self = weak_self.lock()) {
            self->HandlePollResult(res);
        }
    });
}

void PollingDataSource::HandlePollResult(network::HttpResult const& res) {
    auto header_etag = res.Headers().find("etag");
    bool has_etag = header_etag != res.Headers().end();

    if (etag_ && has_etag) {
        if (etag_.value() == header_etag->second) {
            // Got the same etag; we know the content has not changed.
            // So we can just start the next timer.

            // We don't need to update the "request_" because it would have
            // the same Etag.
            StartPollingTimer();
            return;
        }
    }

    if (has_etag) {
        config::shared::builders::HttpPropertiesBuilder<
            config::shared::ServerSDK>
            builder(request_.Properties());
        builder.Header("If-None-Match", header_etag->second);
        request_ = network::HttpRequest(request_, builder.Build());

        etag_ = header_etag->second;
    }

    if (res.IsError()) {
        status_manager_.SetState(
            DataSourceStatus::DataSourceState::kInterrupted,
            DataSourceStatus::ErrorInfo::ErrorKind::kNetworkError,
            res.ErrorMessage().has_value() ? *res.ErrorMessage()
                                           : "unknown error");
        LD_LOG(logger_, LogLevel::kWarn)
            << "Polling for feature flag updates failed: "
            << (res.ErrorMessage().has_value() ? *res.ErrorMessage()
                                               : "unknown error");
    } else if (res.Status() == 200) {
        if (res.Body().has_value()) {
            boost::json::error_code error_code;
            auto parsed = boost::json::parse(res.Body().value(), error_code);
            if (error_code) {
                LD_LOG(logger_, LogLevel::kError) << kErrorParsingPut;
                status_manager_.SetError(
                    DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
                    kErrorParsingPut);
                return;
            }
            auto poll_result = boost::json::value_to<
                tl::expected<data_model::SDKDataSet, JsonError>>(parsed);

            if (poll_result.has_value()) {
                update_sink_.Init(std::move(*poll_result));
                status_manager_.SetState(
                    DataSourceStatus::DataSourceState::kValid);
                return;
            }
            LD_LOG(logger_, LogLevel::kError) << kErrorPutInvalid;
            status_manager_.SetError(
                DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
                kErrorPutInvalid);
            return;
        } else {
            status_manager_.SetState(
                DataSourceStatus::DataSourceState::kInterrupted,
                DataSourceStatus::ErrorInfo::ErrorKind::kUnknown,
                "polling response contained no body.");
        }
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
                DataSourceStatus::DataSourceState::kOff, res.Status(),
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
            // The timer was canceled. Stop polling.
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
            DataSourceStatus::DataSourceState::kOff,
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

}  // namespace launchdarkly::server_side::data_sources
