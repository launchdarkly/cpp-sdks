#include "contract_test_hook.hpp"
#include "http_callback.hpp"

#include <launchdarkly/detail/serialization/json_primitives.hpp>
#include <launchdarkly/detail/serialization/json_value.hpp>
#include <launchdarkly/serialization/json_context.hpp>
#include <launchdarkly/serialization/json_evaluation_reason.hpp>

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
    PostJsonCallback(executor_, config_.callbackUri, payload);
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
    ctx["context"] = nlohmann::json::parse(boost::json::serialize(
        boost::json::value_from(series_context.EvaluationContext())));
    ctx["defaultValue"] = nlohmann::json::parse(boost::json::serialize(
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
    ctx["context"] = nlohmann::json::parse(boost::json::serialize(
        boost::json::value_from(series_context.EvaluationContext())));
    ctx["defaultValue"] = nlohmann::json::parse(boost::json::serialize(
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

void ContractTestHook::AfterTrack(TrackSeriesContext const& series_context) {
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
    ctx["context"] = nlohmann::json::parse(boost::json::serialize(
        boost::json::value_from(series_context.TrackContext())));
    ctx["key"] = std::string(series_context.Key());
    if (series_context.MetricValue()) {
        ctx["metricValue"] = *series_context.MetricValue();
    }
    if (series_context.Data()) {
        ctx["data"] = nlohmann::json::parse(boost::json::serialize(
            boost::json::value_from(series_context.Data()->get())));
    }
    if (series_context.EnvironmentId()) {
        ctx["environmentId"] = std::string(*series_context.EnvironmentId());
    }
    payload["trackSeriesContext"] = ctx;

    PostCallback("afterTrack", payload);
}
