#include "polling_data_source.hpp"

#include <launchdarkly/encoding/base_64.hpp>
#include <launchdarkly/network/http_error_messages.hpp>

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_primitives.hpp>
#include <launchdarkly/serialization/json_sdk_data_set.hpp>
#include <launchdarkly/server_side/data_source_status.hpp>

#include <launchdarkly/server_side/config/builders/all_builders.hpp>

#include <boost/json.hpp>

namespace launchdarkly::server_side::data_systems {
static char const* const kErrorParsingPut = "Could not parse polling payload";
static char const* const kErrorPutInvalid =
    "Polling payload contained invalid data";

static char const* const kCouldNotParseEndpoint =
    "Could not parse polling endpoint URL";

static network::HttpRequest MakeRequest(
    config::built::BackgroundSyncConfig::PollingConfig const& polling_config,
    config::built::ServiceEndpoints const& endpoints,
    config::built::HttpProperties const& http_properties) {
    auto url = std::make_optional(endpoints.PollingBaseUrl());

    url = network::AppendUrl(url, polling_config.polling_get_path);

    network::HttpRequest::BodyType body;
    network::HttpMethod method = network::HttpMethod::kGet;

    config::builders::HttpPropertiesBuilder const builder(http_properties);

    // If no URL is set, then we will fail the request.
    return {url.value_or(""), method, builder.Build(), body};
}

std::string const& PollingDataSource::Identity() const {
    static std::string const identity = "polling data source";
    return identity;
}

PollingDataSource::PollingDataSource(
    boost::asio::any_io_executor const& ioc,
    Logger const& logger,
    data_components::DataSourceStatusManager& status_manager,
    config::built::ServiceEndpoints const& endpoints,
    config::built::BackgroundSyncConfig::PollingConfig const&
        data_source_config,
    config::built::HttpProperties const& http_properties)
    : logger_(logger),
      status_manager_(status_manager),
      requester_(ioc, http_properties.Tls().PeerVerifyMode()),
      polling_interval_(data_source_config.poll_interval),
      request_(MakeRequest(data_source_config, endpoints, http_properties)),
      timer_(ioc),
      sink_(nullptr) {
    if (http_properties.Tls().PeerVerifyMode() ==
        launchdarkly::config::shared::built::TlsOptions::VerifyMode::
            kVerifyNone) {
        LD_LOG(logger_, LogLevel::kDebug) << "TLS peer verification disabled";
    }
    if (polling_interval_ < data_source_config.min_polling_interval) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "Polling interval too frequent, defaulting to "
            << std::chrono::duration_cast<std::chrono::seconds>(
                   data_source_config.min_polling_interval)
                   .count()
            << " seconds";

        polling_interval_ = data_source_config.min_polling_interval;
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
        config::builders::HttpPropertiesBuilder builder(request_.Properties());
        builder.Header("If-None-Match", header_etag->second);
        request_ = network::HttpRequest(request_, builder.Build());

        etag_ = header_etag->second;
    }

    if (res.IsError()) {
        auto const& error_message = res.ErrorMessage();
        status_manager_.SetState(
            DataSourceStatus::DataSourceState::kInterrupted,
            DataSourceStatus::ErrorInfo::ErrorKind::kNetworkError,
            error_message.has_value() ? *error_message : "unknown error");
        LD_LOG(logger_, LogLevel::kWarn)
            << "Polling for feature flag updates failed: "
            << (error_message.has_value() ? *error_message : "unknown error");
    } else if (res.Status() == 200) {
        auto const& body = res.Body();
        if (body.has_value()) {
            boost::json::error_code error_code;
            auto parsed = boost::json::parse(body.value(), error_code);
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
                sink_->Init(std::move(*poll_result));
                status_manager_.SetState(
                    DataSourceStatus::DataSourceState::kValid);
                return;
            }
            LD_LOG(logger_, LogLevel::kError) << kErrorPutInvalid;
            status_manager_.SetError(
                DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
                kErrorPutInvalid);
            return;
        }
        status_manager_.SetState(
            DataSourceStatus::DataSourceState::kInterrupted,
            DataSourceStatus::ErrorInfo::ErrorKind::kUnknown,
            "polling response contained no body.");

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

void PollingDataSource::StartAsync(
    data_interfaces::IDestination* dest,
    data_model::SDKDataSet const* bootstrap_data) {
    boost::ignore_unused(bootstrap_data);

    sink_ = dest;

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

}  // namespace launchdarkly::server_side::data_systems
