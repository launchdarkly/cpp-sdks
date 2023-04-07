#include <boost/asio/io_context.hpp>
#include <boost/json.hpp>

#include <utility>

#include "launchdarkly/detail/base_64.hpp"
#include "launchdarkly/detail/streaming_data_source.hpp"
#include "serialization/json_context.hpp"
#include "serialization/json_evaluation_result.hpp"
#include "serialization/value_mapping.hpp"

namespace launchdarkly::client_side {

tl::expected<StreamingDataSource::PatchData, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<StreamingDataSource::PatchData, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (json_value.is_object()) {
        auto const& obj = json_value.as_object();
        auto const* key_iter = obj.find("key");
        auto key = ValueAsOpt<std::string>(key_iter, obj.end());
        auto result =
            boost::json::value_to<tl::expected<EvaluationResult, JsonError>>(
                json_value);

        if (result.has_value() && key.has_value()) {
            return StreamingDataSource::PatchData{key.value(), result.value()};
        }
    }
    return tl::unexpected(JsonError::kSchemaFailure);
}

tl::expected<StreamingDataSource::DeleteData, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<StreamingDataSource::DeleteData, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (json_value.is_object()) {
        auto const& obj = json_value.as_object();
        auto const* key_iter = obj.find("key");
        auto key = ValueAsOpt<std::string>(key_iter, obj.end());
        auto const* version_iter = obj.find("version");
        std::optional<uint64_t> version =
            ValueAsOpt<uint64_t>(version_iter, obj.end());

        if (key.has_value() && version.has_value()) {
            return StreamingDataSource::DeleteData{key.value(),
                                                   version.value()};
        }
    }
    return tl::unexpected(JsonError::kSchemaFailure);
}

tl::expected<std::map<std::string, ItemDescriptor>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::map<std::string, ItemDescriptor>, JsonError>> const&
        unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (!json_value.is_object()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    auto const& obj = json_value.as_object();
    std::map<std::string, ItemDescriptor> descriptors;
    for (auto const& pair : obj) {
        auto eval_result =
            boost::json::value_to<tl::expected<EvaluationResult, JsonError>>(
                pair.value());
        if (!eval_result.has_value()) {
            return tl::unexpected(JsonError::kSchemaFailure);
        }
        descriptors.emplace(pair.key(),
                            ItemDescriptor{eval_result.value().version(),
                                           std::move(eval_result.value())});
    }
    return descriptors;
}

StreamingDataSource::StreamingDataSource(
    boost::asio::any_io_executor executor,
    Context const& context,
    bool use_report,
    bool with_reasons,
    std::chrono::duration<int, std::milli> initial_retry_delay,
    config::ServiceEndpoints const& endpoints,
    HttpProperties const& http_properties,
    std::shared_ptr<IDataSourceUpdateSink> handler,
    Logger const& logger)
    : executor_(std::move(std::move(executor))),
      handler_(std::move(std::move(handler))),
      logger_(logger) {
    // TODO: Use boost to build the URL.
    auto string_context =
        boost::json::serialize(boost::json::value_from(context));
    if (use_report) {
        string_context_ = string_context;
        streaming_endpoint_ = endpoints.streaming_base_url() + streaming_path_;
    } else {
        // When not using 'REPORT' we need to base64 encode the context
        // so that we can safely put it in a url.
        string_context_ = Base64UrlEncode(string_context);
        streaming_endpoint_ = endpoints.streaming_base_url() + streaming_path_ +
                              "/" + string_context_;
    }
    if (with_reasons) {
        streaming_endpoint_ += "?withReasons=true";
    }

    auto client_builder =
        launchdarkly::sse::Builder(executor_, streaming_endpoint_);

    client_builder.method(use_report ? boost::beast::http::verb::report
                                     : boost::beast::http::verb::get);

    client_builder.receiver([this](launchdarkly::sse::Event const& event) {
        handle_message(event);
    });

    client_builder.logger(
        [this](auto msg) { LD_LOG(logger_, LogLevel::kInfo) << msg; });

    if (use_report) {
        client_builder.body(string_context_);
    }
    client_builder.header("authorization", std::getenv("STG_SDK_KEY"));
    client_ = client_builder.build();
}

void StreamingDataSource::start() {
    client_->run();
}

void StreamingDataSource::close() {
    client_->close();
}

void StreamingDataSource::handle_message(
    launchdarkly::sse::Event const& event) {
    if (event.type() == "put") {
        boost::json::error_code error_code;
        auto parsed = boost::json::parse(event.data(), error_code);
        if (error_code) {
            LD_LOG(logger_, LogLevel::kError) << "Could not parse PUT message";
        } else {
            auto res = boost::json::value_to<
                tl::expected<std::map<std::string, ItemDescriptor>, JsonError>>(
                parsed);

            if (res.has_value()) {
                handler_->init(res.value());
            } else {
                LD_LOG(logger_, LogLevel::kError)
                    << "PUT message contained invalid data";
            }
        }

    } else if (event.type() == "patch") {
        boost::json::error_code error_code;
        auto parsed = boost::json::parse(event.data(), error_code);
        if (error_code) {
            LD_LOG(logger_, LogLevel::kError)
                << "Could not parse PATCH message";
        } else {
            auto res = boost::json::value_to<
                tl::expected<StreamingDataSource::PatchData, JsonError>>(
                parsed);
            if (res.has_value()) {
                handler_->upsert(res.value().key,
                                 ItemDescriptor{res.value().flag.version(),
                                                res.value().flag});
            } else {
                LD_LOG(logger_, LogLevel::kError)
                    << "PATCH message contained invalid data";
            }
        }
    } else if (event.type() == "delete") {
        boost::json::error_code error_code;
        auto parsed = boost::json::parse(event.data(), error_code);
        if (error_code) {
            LD_LOG(logger_, LogLevel::kError)
                << "Could not parse DELETE message";
        } else {
            auto res = boost::json::value_to<
                tl::expected<StreamingDataSource::DeleteData, JsonError>>(
                boost::json::parse(event.data()));
            if (res.has_value()) {
                handler_->upsert(
                    res.value().key,
                    ItemDescriptor{res.value().version, std::nullopt});
            } else {
                LD_LOG(logger_, LogLevel::kError)
                    << "DELETE message contained invalid data";
            }
        }
    }
}

}  // namespace launchdarkly::client_side
