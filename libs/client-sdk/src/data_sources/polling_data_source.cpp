#include <boost/json.hpp>
#include <boost/url.hpp>

#include "config/detail/builders/http_properties_builder.hpp"
#include "config/detail/sdks.hpp"
#include "launchdarkly/client_side/data_sources/detail/base_64.hpp"
#include "launchdarkly/client_side/data_sources/detail/polling_data_source.hpp"
#include "serialization/json_context.hpp"

namespace launchdarkly::client_side::data_sources::detail {

static network::detail::HttpRequest MakeRequest(
    std::string const& sdk_key,
    Context const& context,
    config::detail::built::HttpProperties const& http_properties,
    config::detail::built::ServiceEndpoints const& endpoints,
    bool use_report,
    bool with_reasons,
    std::string const& polling_report_path,
    std::string const& polling_get_path) {
    std::string url = endpoints.PollingBaseUrl();

    auto string_context =
        boost::json::serialize(boost::json::value_from(context));

    // TODO: Handle slashes.

    network::detail::HttpRequest::BodyType body;
    network::detail::HttpMethod method = network::detail::HttpMethod::kGet;

    if (use_report) {
        url.append(polling_report_path);
        method = network::detail::HttpMethod::kReport;
        body = string_context;
    } else {
        url.append(polling_get_path);
        // When not using 'REPORT' we need to base64
        // encode the context so that we can safely
        // put it in a url.
        url.append("/" + Base64UrlEncode(string_context));
    }

    if (with_reasons) {
        url.append("?withReasons=true");
    }

    config::detail::builders::HttpPropertiesBuilder<config::detail::ClientSDK>
        builder(http_properties);

    builder.Header("authorization", sdk_key);

    return network::detail::HttpRequest(url, method, builder.Build(), body);
}

PollingDataSource::PollingDataSource(
    std::string const& sdk_key,
    boost::asio::any_io_executor ioc,
    Context const& context,
    config::detail::built::ServiceEndpoints const& endpoints,
    config::detail::built::HttpProperties const& http_properties,
    std::chrono::seconds polling_interval,
    bool use_report,
    bool with_reasons,
    IDataSourceUpdateSink* handler,
    DataSourceStatusManager& status_manager,
    Logger const& logger)
    : ioc_(ioc),
      logger_(logger),
      status_manager_(status_manager),
      data_source_handler_(
          DataSourceEventHandler(handler, logger, status_manager_)),
      requester_(ioc),
      timer_(ioc),
      polling_interval_(polling_interval),
      request_(MakeRequest(sdk_key,
                           context,
                           http_properties,
                           endpoints,
                           use_report,
                           with_reasons,
                           polling_report_path_,
                           polling_get_path_)) {}

void PollingDataSource::DoPoll() {
    LD_LOG(logger_, LogLevel::kInfo) << "Starting poll";
    requester_.Request(request_, [this](network::detail::HttpResult res) {
        LD_LOG(logger_, LogLevel::kInfo) << "Poll complete: " << res;
        // TODO: Retry support.

        auto header_etag = res.Headers().find("etag");
        bool has_etag = header_etag != res.Headers().end();

        LD_LOG(logger_, LogLevel::kInfo) << "has_etag: " << has_etag;
        LD_LOG(logger_, LogLevel::kInfo)
            << "has local etag: " << etag_.has_value();

        if (etag_ && has_etag) {
            LD_LOG(logger_, LogLevel::kInfo) << "Comparing Etag.";
            if (etag_.value() == header_etag->second) {
                LD_LOG(logger_, LogLevel::kInfo)
                    << "Etag matched. Start next poll";
                // Got the same etag, we know the content has not changed.
                // So we can just start the next timer.

                // We don't need to update the "request_" because it would have
                // the same Etag.
                StartPollingTimer();
                return;
            }
        }

        if (has_etag) {
            LD_LOG(logger_, LogLevel::kInfo) << "Updating Etag" << res.Status();
            config::detail::builders::HttpPropertiesBuilder<
                config::detail::ClientSDK>
                builder(request_.Properties());
            builder.Header("If-None-Match", header_etag->second);
            request_ = network::detail::HttpRequest(request_, builder.Build());

            etag_ = header_etag->second;
        }

        if (res.IsError()) {
            // TODO: Do something
            LD_LOG(logger_, LogLevel::kError) << "Got error result" << res;
        }
        if (res.Status() == 200 || res.Status() == 304) {
            data_source_handler_.HandleMessage("put", res.Body().value());
        } else {
            // TODO: Do something
        }

        StartPollingTimer();
    });
}
void PollingDataSource::StartPollingTimer() {
    // TODO: Calculate interval based on request time.
    timer_.cancel();
    timer_.expires_after(polling_interval_);

    timer_.async_wait([this](boost::system::error_code const& ec) {
        if (ec) {
            // TODO: Something;
            return;
        }
        LD_LOG(logger_, LogLevel::kInfo) << "Timer elapsed";
        DoPoll();
    });
}

void PollingDataSource::Start() {
    DoPoll();
}

void PollingDataSource::Close() {
    timer_.cancel();
}
}  // namespace launchdarkly::client_side::data_sources::detail
