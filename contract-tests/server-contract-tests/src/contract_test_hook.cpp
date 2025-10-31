#include "contract_test_hook.hpp"

#include <launchdarkly/serialization/json_context.hpp>
#include <launchdarkly/serialization/json_evaluation_reason.hpp>
#include <launchdarkly/detail/serialization/json_value.hpp>
#include <launchdarkly/detail/serialization/json_primitives.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/post.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/url.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

using namespace launchdarkly::server_side::hooks;

ContractTestHook::ContractTestHook(boost::asio::any_io_executor executor,
                                   ConfigHookInstance config)
    : executor_(std::move(executor)),
      config_(std::move(config)),
      metadata_(config_.name) {}

HookMetadata const& ContractTestHook::Metadata() const {
    return metadata_;
}

std::optional<std::unordered_map<std::string, nlohmann::json>>
ContractTestHook::GetDataForStage(std::string const& stage) const {
    if (!config_.data) {
        return std::nullopt;
    }
    auto it = config_.data->find(stage);
    if (it == config_.data->end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<std::string> ContractTestHook::GetErrorForStage(
    std::string const& stage) const {
    if (!config_.errors) {
        return std::nullopt;
    }
    auto it = config_.errors->find(stage);
    if (it == config_.errors->end()) {
        return std::nullopt;
    }
    return it->second;
}

void ContractTestHook::PostCallback(std::string const& stage,
                                    nlohmann::json const& payload) {
    // Parse the callback URI
    auto uri_result = boost::urls::parse_uri(config_.callbackUri);
    if (!uri_result) {
        // Invalid URI, skip callback
        return;
    }

    auto uri = *uri_result;
    std::string host(uri.host());
    std::string port = uri.has_port() ? std::string(uri.port()) : "80";
    std::string target(uri.path());
    if (target.empty()) {
        target = "/";
    }

    std::string body = payload.dump();

    // Create a shared connection and post asynchronously without blocking
    // This uses fire-and-forget semantics - we don't wait for response
    auto conn = std::make_shared<beast::tcp_stream>(executor_);
    auto resolver = std::make_shared<tcp::resolver>(executor_);

    // Capture everything needed by value in shared_ptrs
    auto req = std::make_shared<http::request<http::string_body>>();
    req->method(http::verb::post);
    req->target(target);
    req->version(11);
    req->set(http::field::host, host);
    req->set(http::field::user_agent, "cpp-server-sdk-contract-tests");
    req->set(http::field::content_type, "application/json");
    req->body() = body;
    req->prepare_payload();

    // Start async resolution
    resolver->async_resolve(host, port,
        [conn, resolver, req](beast::error_code ec, tcp::resolver::results_type results) {
            if (ec) return; // Silently ignore resolution errors

            // Connect asynchronously
            conn->async_connect(results,
                [conn, resolver, req](beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
                    if (ec) return; // Silently ignore connection errors

                    // Write request asynchronously
                    http::async_write(*conn, *req,
                        [conn, resolver, req](beast::error_code ec, std::size_t) {
                            if (ec) return; // Silently ignore write errors

                            // Read response asynchronously (but don't wait for result)
                            auto res = std::make_shared<http::response<http::string_body>>();
                            auto buffer = std::make_shared<beast::flat_buffer>();
                            http::async_read(*conn, *buffer, *res,
                                [conn, resolver, req, res, buffer](beast::error_code ec, std::size_t) {
                                    // Close connection gracefully
                                    conn->socket().shutdown(tcp::socket::shutdown_both, ec);
                                    // Cleanup happens automatically via shared_ptr
                                });
                        });
                });
        });
}

EvaluationSeriesData ContractTestHook::BeforeEvaluation(
    EvaluationSeriesContext const& series_context,
    EvaluationSeriesData data) {

    // Check if we should throw an error for this stage
    auto error = GetErrorForStage("beforeEvaluation");
    if (error) {
        throw std::runtime_error(*error);
    }

    // Build the evaluation series data from configured data
    EvaluationSeriesDataBuilder builder(data);
    auto stage_data = GetDataForStage("beforeEvaluation");
    if (stage_data) {
        for (auto const& [key, value] : *stage_data) {
            // Convert nlohmann::json to launchdarkly::Value via boost::json
            auto maybe_value = boost::json::value_to<
                tl::expected<launchdarkly::Value, launchdarkly::JsonError>>(
                boost::json::parse(value.dump()));
            if (maybe_value) {
                builder.Set(key, *maybe_value);
            }
        }
    }

    // Build payload for callback
    nlohmann::json payload;
    payload["stage"] = "beforeEvaluation";

    // EvaluationSeriesContext
    nlohmann::json ctx;
    ctx["flagKey"] = std::string(series_context.FlagKey());
    ctx["context"] = nlohmann::json::parse(
        boost::json::serialize(
            boost::json::value_from(series_context.EvaluationContext())));
    ctx["defaultValue"] = nlohmann::json::parse(
        boost::json::serialize(
            boost::json::value_from(series_context.DefaultValue())));
    ctx["method"] = std::string(series_context.Method());
    if (series_context.EnvironmentId()) {
        ctx["environmentId"] = std::string(*series_context.EnvironmentId());
    }
    payload["evaluationSeriesContext"] = ctx;

    // EvaluationSeriesData
    nlohmann::json series_data_json;
    for (auto const& key : data.Keys()) {
        auto val = data.Get(key);
        if (val) {
            series_data_json[key] = nlohmann::json::parse(
                boost::json::serialize(boost::json::value_from(val->get())));
        }
    }
    payload["evaluationSeriesData"] = series_data_json;

    PostCallback("beforeEvaluation", payload);

    return builder.Build();
}

EvaluationSeriesData ContractTestHook::AfterEvaluation(
    EvaluationSeriesContext const& series_context,
    EvaluationSeriesData data,
    launchdarkly::EvaluationDetail<launchdarkly::Value> const& detail) {

    // Check if we should throw an error for this stage
    auto error = GetErrorForStage("afterEvaluation");
    if (error) {
        throw std::runtime_error(*error);
    }

    // Build payload for callback
    nlohmann::json payload;
    payload["stage"] = "afterEvaluation";

    // EvaluationSeriesContext
    nlohmann::json ctx;
    ctx["flagKey"] = std::string(series_context.FlagKey());
    ctx["context"] = nlohmann::json::parse(
        boost::json::serialize(
            boost::json::value_from(series_context.EvaluationContext())));
    ctx["defaultValue"] = nlohmann::json::parse(
        boost::json::serialize(
            boost::json::value_from(series_context.DefaultValue())));
    ctx["method"] = std::string(series_context.Method());
    if (series_context.EnvironmentId()) {
        ctx["environmentId"] = std::string(*series_context.EnvironmentId());
    }
    payload["evaluationSeriesContext"] = ctx;

    // EvaluationSeriesData
    nlohmann::json series_data_json;
    for (auto const& key : data.Keys()) {
        auto val = data.Get(key);
        if (val) {
            series_data_json[key] = nlohmann::json::parse(
                boost::json::serialize(boost::json::value_from(val->get())));
        }
    }
    payload["evaluationSeriesData"] = series_data_json;

    // EvaluationDetail
    nlohmann::json detail_json;
    detail_json["value"] = nlohmann::json::parse(
        boost::json::serialize(boost::json::value_from(detail.Value())));
    if (detail.VariationIndex()) {
        detail_json["variationIndex"] = *detail.VariationIndex();
    }
    if (detail.Reason()) {
        detail_json["reason"] = nlohmann::json::parse(
            boost::json::serialize(boost::json::value_from(*detail.Reason())));
    }
    payload["evaluationDetail"] = detail_json;

    PostCallback("afterEvaluation", payload);

    return data;
}

void ContractTestHook::AfterTrack(
    TrackSeriesContext const& series_context) {

    // Check if we should throw an error for this stage
    auto error = GetErrorForStage("afterTrack");
    if (error) {
        throw std::runtime_error(*error);
    }

    // Build payload for callback
    nlohmann::json payload;
    payload["stage"] = "afterTrack";

    // TrackSeriesContext
    nlohmann::json ctx;
    ctx["context"] = nlohmann::json::parse(
        boost::json::serialize(
            boost::json::value_from(series_context.TrackContext())));
    ctx["key"] = std::string(series_context.Key());
    if (series_context.MetricValue()) {
        ctx["metricValue"] = *series_context.MetricValue();
    }
    if (series_context.Data()) {
        ctx["data"] = nlohmann::json::parse(
            boost::json::serialize(
                boost::json::value_from(series_context.Data()->get())));
    }
    if (series_context.EnvironmentId()) {
        ctx["environmentId"] = std::string(*series_context.EnvironmentId());
    }
    payload["trackSeriesContext"] = ctx;

    PostCallback("afterTrack", payload);
}
