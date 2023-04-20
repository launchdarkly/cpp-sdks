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
#include "serialization/json_context.hpp"

namespace launchdarkly::client_side::data_sources::detail {

StreamingDataSource::StreamingDataSource(
    std::string const& sdk_key,
    boost::asio::any_io_executor ioc,
    Context const& context,
    config::detail::built::ServiceEndpoints const& endpoints,
    config::detail::built::HttpProperties const& http_properties,
    bool use_report,
    bool with_reasons,
    IDataSourceUpdateSink* handler,
    DataSourceStatusManager& status_manager,
    Logger const& logger)
    : logger_(logger),
      status_manager_(status_manager),
      data_source_handler_(
          StreamingDataHandler(handler, logger, status_manager_)) {
    auto uri_components = boost::urls::parse_uri(endpoints.StreamingBaseUrl());

    // TODO: Handle parsing error?
    boost::urls::url url = uri_components.value();

    auto string_context =
        boost::json::serialize(boost::json::value_from(context));

    // Add the eval endpoint.
    url.set_path(url.path().append(streaming_path_));

    if (!use_report) {
        // When not using 'REPORT' we need to base64
        // encode the context so that we can safely
        // put it in a url.
        url.set_path(url.path().append("/" + Base64UrlEncode(string_context)));
    }
    if (with_reasons) {
        url.params().set("withReasons", "true");
    }

    auto client_builder =
        launchdarkly::sse::Builder(std::move(ioc), url.buffer());

    client_builder.method(use_report ? boost::beast::http::verb::report
                                     : boost::beast::http::verb::get);

    client_builder.receiver([this](launchdarkly::sse::Event const& event) {
        data_source_handler_.HandleMessage(event);
        // TODO: Use the result of handle message to restart the
        // event source if we got bad data.
    });

    client_builder.logger(
        [this](auto msg) { LD_LOG((logger_), LogLevel::kInfo) << msg; });

    if (use_report) {
        client_builder.body(string_context);
    }
    client_builder.header("authorization", sdk_key);
    for (auto const& header : http_properties.BaseHeaders()) {
        client_builder.header(header.first, header.second);
    }
    client_builder.header("user-agent", http_properties.UserAgent());
    // TODO: Handle proxy support.
    client_ = client_builder.build();
}

void StreamingDataSource::Start() {
    client_->run();
}

void StreamingDataSource::Close() {
    status_manager_.SetState(DataSourceStatus::DataSourceState::kShutdown);
    client_->close();
}

}  // namespace launchdarkly::client_side::data_sources::detail
