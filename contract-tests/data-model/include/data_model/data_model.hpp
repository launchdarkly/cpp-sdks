#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include "nlohmann/json.hpp"

namespace nlohmann {

template <typename T>
struct adl_serializer<std::optional<T>> {
    static void to_json(json& j, std::optional<T> const& opt) {
        if (opt == std::nullopt) {
            j = nullptr;
        } else {
            j = *opt;  // this will call adl_serializer<T>::to_json which will
            // find the free function to_json in T's namespace!
        }
    }

    static void from_json(json const& j, std::optional<T>& opt) {
        if (j.is_null()) {
            opt = std::nullopt;
        } else {
            opt = j.get<T>();  // same as above, but with
            // adl_serializer<T>::from_json
        }
    }
};
}  // namespace nlohmann

struct ConfigTLSParams {
    std::optional<bool> skipVerifyPeer;
    std::optional<std::string> customCAPath;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ConfigTLSParams,
                                                skipVerifyPeer,
                                                customCAPath);

struct ConfigStreamingParams {
    std::optional<std::string> baseUri;
    std::optional<uint32_t> initialRetryDelayMs;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ConfigStreamingParams,
                                                baseUri,
                                                initialRetryDelayMs);

struct ConfigPollingParams {
    std::optional<std::string> baseUri;
    std::optional<uint32_t> pollIntervalMs;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ConfigPollingParams,
                                                baseUri,
                                                pollIntervalMs);

struct ConfigEventParams {
    std::optional<std::string> baseUri;
    std::optional<uint32_t> capacity;
    std::optional<bool> enableDiagnostics;
    std::optional<bool> allAttributesPrivate;
    std::vector<std::string> globalPrivateAttributes;
    std::optional<int> flushIntervalMs;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ConfigEventParams,
                                                baseUri,
                                                capacity,
                                                enableDiagnostics,
                                                allAttributesPrivate,
                                                globalPrivateAttributes,
                                                flushIntervalMs);
struct ConfigServiceEndpointsParams {
    std::optional<std::string> streaming;
    std::optional<std::string> polling;
    std::optional<std::string> events;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ConfigServiceEndpointsParams,
                                                streaming,
                                                polling,
                                                events);

struct ConfigClientSideParams {
    nlohmann::json initialContext;
    std::optional<bool> evaluationReasons;
    std::optional<bool> useReport;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ConfigClientSideParams,
                                                initialContext,
                                                evaluationReasons,
                                                useReport);

struct ConfigTags {
    std::optional<std::string> applicationId;
    std::optional<std::string> applicationVersion;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ConfigTags,
                                                applicationId,
                                                applicationVersion);

struct ConfigParams {
    std::string credential;
    std::optional<uint32_t> startWaitTimeMs;
    std::optional<bool> initCanFail;
    std::optional<ConfigStreamingParams> streaming;
    std::optional<ConfigPollingParams> polling;
    std::optional<ConfigEventParams> events;
    std::optional<ConfigServiceEndpointsParams> serviceEndpoints;
    std::optional<ConfigClientSideParams> clientSide;
    std::optional<ConfigTags> tags;
    std::optional<ConfigTLSParams> tls;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ConfigParams,
                                                credential,
                                                startWaitTimeMs,
                                                initCanFail,
                                                streaming,
                                                polling,
                                                events,
                                                serviceEndpoints,
                                                clientSide,
                                                tags,
                                                tls);

struct ContextSingleParams {
    std::optional<std::string> kind;
    std::string key;
    std::optional<std::string> name;
    std::optional<bool> anonymous;
    std::optional<std::vector<std::string>> _private;
    std::optional<std::unordered_map<std::string, nlohmann::json>> custom;
};

// These are defined manually because of the 'private' field, which is a
// reserved keyword in C++.
inline void to_json(nlohmann::json& nlohmann_json_j,
                    ContextSingleParams const& nlohmann_json_t) {
    nlohmann_json_j["kind"] = nlohmann_json_t.kind;
    nlohmann_json_j["key"] = nlohmann_json_t.key;
    nlohmann_json_j["name"] = nlohmann_json_t.name;
    nlohmann_json_j["anonymous"] = nlohmann_json_t.anonymous;
    nlohmann_json_j["private"] = nlohmann_json_t._private;
    nlohmann_json_j["custom"] = nlohmann_json_t.custom;
}
inline void from_json(nlohmann::json const& nlohmann_json_j,
                      ContextSingleParams& nlohmann_json_t) {
    ContextSingleParams nlohmann_json_default_obj;
    nlohmann_json_t.kind =
        nlohmann_json_j.value("kind", nlohmann_json_default_obj.kind);
    nlohmann_json_t.key =
        nlohmann_json_j.value("key", nlohmann_json_default_obj.key);
    nlohmann_json_t.name =
        nlohmann_json_j.value("name", nlohmann_json_default_obj.name);
    nlohmann_json_t.anonymous =
        nlohmann_json_j.value("anonymous", nlohmann_json_default_obj.anonymous);
    nlohmann_json_t._private =
        nlohmann_json_j.value("private", nlohmann_json_default_obj._private);
    nlohmann_json_t.custom =
        nlohmann_json_j.value("custom", nlohmann_json_default_obj.custom);
}

struct ContextBuildParams {
    std::optional<ContextSingleParams> single;
    std::optional<std::vector<ContextSingleParams>> multi;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ContextBuildParams,
                                                single,
                                                multi);

struct ContextConvertParams {
    std::string input;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ContextConvertParams, input);

struct ContextResponse {
    std::optional<std::string> output;
    std::optional<std::string> error;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ContextResponse, output, error);

struct CreateInstanceParams {
    ConfigParams configuration;
    std::string tag;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CreateInstanceParams,
                                                configuration,
                                                tag);

enum class ValueType { Bool = 1, Int, Double, String, Any, Unspecified };
NLOHMANN_JSON_SERIALIZE_ENUM(ValueType,
                             {{ValueType::Bool, "bool"},
                              {ValueType::Int, "int"},
                              {ValueType::Double, "double"},
                              {ValueType::String, "string"},
                              {ValueType::Any, "any"},
                              {ValueType::Unspecified, ""}})

struct EvaluateFlagParams {
    std::string flagKey;
    std::optional<nlohmann::json> context;
    ValueType valueType;
    nlohmann::json defaultValue;
    bool detail;
    EvaluateFlagParams();
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(EvaluateFlagParams,
                                                flagKey,
                                                context,
                                                valueType,
                                                defaultValue,
                                                detail);

struct EvaluateFlagResponse {
    nlohmann::json value;
    std::optional<uint32_t> variationIndex;
    std::optional<nlohmann::json> reason;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(EvaluateFlagResponse,
                                                value,
                                                variationIndex,
                                                reason);

struct EvaluateAllFlagParams {
    std::optional<nlohmann::json> context;
    std::optional<bool> withReasons;
    std::optional<bool> clientSideOnly;
    std::optional<bool> detailsOnlyForTrackedFlags;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(EvaluateAllFlagParams,
                                                context,
                                                withReasons,
                                                clientSideOnly,
                                                detailsOnlyForTrackedFlags);
struct EvaluateAllFlagsResponse {
    nlohmann::json state;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(EvaluateAllFlagsResponse,
                                                state);

struct CustomEventParams {
    std::string eventKey;
    std::optional<nlohmann::json> context;
    std::optional<nlohmann::json> data;
    std::optional<bool> omitNullData;
    std::optional<double> metricValue;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CustomEventParams,
                                                eventKey,
                                                context,
                                                data,
                                                omitNullData,
                                                metricValue);

struct IdentifyEventParams {
    nlohmann::json context;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(IdentifyEventParams, context);

enum class Command {
    Unknown = -1,
    EvaluateFlag,
    EvaluateAllFlags,
    IdentifyEvent,
    CustomEvent,
    FlushEvents,
    ContextBuild,
    ContextConvert
};
NLOHMANN_JSON_SERIALIZE_ENUM(Command,
                             {{Command::Unknown, nullptr},
                              {Command::EvaluateFlag, "evaluate"},
                              {Command::EvaluateAllFlags, "evaluateAll"},
                              {Command::IdentifyEvent, "identifyEvent"},
                              {Command::CustomEvent, "customEvent"},
                              {Command::FlushEvents, "flushEvents"},
                              {Command::ContextBuild, "contextBuild"},
                              {Command::ContextConvert, "contextConvert"}});

struct CommandParams {
    Command command;
    std::optional<EvaluateFlagParams> evaluate;
    std::optional<EvaluateAllFlagParams> evaluateAll;
    std::optional<CustomEventParams> customEvent;
    std::optional<IdentifyEventParams> identifyEvent;
    std::optional<ContextBuildParams> contextBuild;
    std::optional<ContextConvertParams> contextConvert;
    CommandParams();
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CommandParams,
                                                command,
                                                evaluate,
                                                evaluateAll,
                                                customEvent,
                                                identifyEvent,
                                                contextBuild,
                                                contextConvert);
