#include "client_entity.hpp"
#include <boost/json.hpp>
#include "serialization/json_evaluation_reason.hpp"
#include "serialization/json_value.hpp"
#include "value.hpp"

ClientEntity::ClientEntity(
    std::unique_ptr<launchdarkly::client_side::Client> client)
    : client_(std::move(client)) {}

tl::expected<nlohmann::json, std::string> ClientEntity::Identify(
    IdentifyEventParams params) {
    return tl::make_unexpected("identify not yet supported");
}

tl::expected<nlohmann::json, std::string> ClientEntity::Custom(
    CustomEventParams params) {
    auto data = params.data ? boost::json::value_to<launchdarkly::Value>(
                                  boost::json::parse(params.data->dump()))
                            : launchdarkly::Value::null();

    if (params.omitNullData.value_or(false) && !params.metricValue &&
        !params.data) {
        client_->Track(params.eventKey);
        return nlohmann::json{};
    }

    if (!params.metricValue) {
        client_->Track(params.eventKey, std::move(data));
        return nlohmann::json{};
    }

    client_->Track(params.eventKey, std::move(data), *params.metricValue);
    return nlohmann::json{};
}

tl::expected<nlohmann::json, std::string> ClientEntity::EvaluateAll(
    EvaluateAllFlagParams params) {
    EvaluateAllFlagsResponse resp;

    return tl::make_unexpected("not yet supported");
}

tl::expected<nlohmann::json, std::string> ClientEntity::EvaluateDetail(
    EvaluateFlagParams params) {
    auto const& key = params.flagKey;

    auto const& defaultVal = params.defaultValue;

    EvaluateFlagResponse result;

    std::optional<launchdarkly::EvaluationReason> reason;

    switch (params.valueType) {
        case ValueType::Bool: {
            auto detail =
                client_->BoolVariationDetail(key, defaultVal.get<bool>());
            result.value = *detail;
            reason = detail.Reason();
            result.variationIndex = detail.VariationIndex();
            break;
        }
        case ValueType::Int: {
            auto detail =
                client_->IntVariationDetail(key, defaultVal.get<int>());
            result.value = *detail;
            reason = detail.Reason();
            result.variationIndex = detail.VariationIndex();
            break;
        }
        case ValueType::Double: {
            auto detail =
                client_->DoubleVariationDetail(key, defaultVal.get<double>());
            result.value = *detail;
            reason = detail.Reason();
            result.variationIndex = detail.VariationIndex();
            break;
        }
        case ValueType::String: {
            auto detail = client_->StringVariationDetail(
                key, defaultVal.get<std::string>());
            result.value = *detail;
            reason = detail.Reason();
            result.variationIndex = detail.VariationIndex();
            break;
        }
        case ValueType::Any:
        case ValueType::Unspecified: {
            auto fallback = boost::json::value_to<launchdarkly::Value>(
                boost::json::parse(defaultVal.dump()));

            /* This switcharoo from nlohmann/json to boost/json to Value, then
             * back is because we're using nlohmann/json for the test harness
             * protocol, but boost::json in the SDK. We could swap over to
             * boost::json entirely here to remove the awkwardness. */

            auto detail = client_->JsonVariationDetail(key, fallback);

            auto serialized =
                boost::json::serialize(boost::json::value_from(*detail));

            result.value = nlohmann::json::parse(serialized);
            reason = detail.Reason();
            result.variationIndex = detail.VariationIndex();
            break;
        }
        default:
            return tl::make_unexpected("unknown variation type");
    }

    result.reason =
        reason.has_value()
            ? std::make_optional(nlohmann::json::parse(
                  boost::json::serialize(boost::json::value_from(*reason))))
            : std::nullopt;

    return result;
}
tl::expected<nlohmann::json, std::string> ClientEntity::Evaluate(
    EvaluateFlagParams params) {
    if (params.detail) {
        return EvaluateDetail(params);
    }

    auto const& key = params.flagKey;

    auto const& defaultVal = params.defaultValue;

    EvaluateFlagResponse result;

    switch (params.valueType) {
        case ValueType::Bool:
            result.value = client_->BoolVariation(key, defaultVal.get<bool>());
            break;
        case ValueType::Int:
            result.value = client_->IntVariation(key, defaultVal.get<int>());
            break;
        case ValueType::Double:
            result.value =
                client_->DoubleVariation(key, defaultVal.get<double>());
            break;
        case ValueType::String: {
            result.value =
                client_->StringVariation(key, defaultVal.get<std::string>());
            break;
        }
        case ValueType::Any:
        case ValueType::Unspecified: {
            auto fallback = boost::json::value_to<launchdarkly::Value>(
                boost::json::parse(defaultVal.dump()));

            /* This switcharoo from nlohmann/json to boost/json to Value, then
             * back is because we're using nlohmann/json for the test harness
             * protocol, but boost::json in the SDK. We could swap over to
             * boost::json entirely here to remove the awkwardness. */

            auto evaluation = client_->JsonVariation(key, fallback);

            auto serialized =
                boost::json::serialize(boost::json::value_from(evaluation));

            result.value = nlohmann::json::parse(serialized);
            break;
        }
        default:
            return tl::make_unexpected("unknown variation type");
    }

    return result;
}
tl::expected<nlohmann::json, std::string> ClientEntity::Command(
    CommandParams params) {
    switch (params.command) {
        case Command::Unknown:
            return tl::make_unexpected("unknown command");
        case Command::EvaluateFlag:
            if (!params.evaluate) {
                return tl::make_unexpected("evaluate params must be set");
            }
            return Evaluate(*params.evaluate);
        case Command::EvaluateAllFlags:
            if (!params.evaluateAll) {
                return tl::make_unexpected("evaluateAll params must be set");
            }
            return EvaluateAll(*params.evaluateAll);
        case Command::IdentifyEvent:
            if (!params.identifyEvent) {
                return tl::make_unexpected("identifyEvent params must be set");
            }
            return Identify(*params.identifyEvent);
        case Command::CustomEvent:
            if (!params.customEvent) {
                return tl::make_unexpected("customEvent params must be set");
            }
            return Custom(*params.customEvent);
        case Command::FlushEvents:
            client_->AsyncFlush();
            return nlohmann::json{};
        case Command::ContextBuild:
            break;
        case Command::ContextConvert:
            break;
    }
    return tl::make_unexpected("unrecognized command");
}
