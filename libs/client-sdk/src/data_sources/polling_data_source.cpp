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
      timer_(nullptr),
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
        LD_LOG(logger_, LogLevel::kInfo) << "Poll complete";
        if (res.IsError()) {
            // TODO: Do something
        }
        if (res.Status() == 200 || res.Status() == 304) {
            data_source_handler_.HandleMessage("put", res.Body().value());
        } else {
            // TODO: Do something
        }

        if (timer_) {
            delete timer_;
            timer_ = nullptr;
        }
        // TODO: Calculate interval based on request time.
        timer_ = new boost::asio::deadline_timer(
            ioc_, boost::posix_time::seconds{polling_interval_.count()});

        timer_->async_wait([this](const boost::system::error_code& ec) {
            if (ec) {
                // TODO: Something;
                return;
            }
            LD_LOG(logger_, LogLevel::kInfo) << "Timer elapsed";
            DoPoll();
        });
    });
}

void PollingDataSource::Start() {
    DoPoll();
}

void PollingDataSource::Close() {
    if (timer_) {
        timer_->cancel();
    }
}
}  // namespace launchdarkly::client_side::data_sources::detail
