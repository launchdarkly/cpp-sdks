#include <launchdarkly/detail/serialization/json_errors.hpp>
#include <launchdarkly/serialization/json_evaluation_reason.hpp>
#include <launchdarkly/serialization/value_mapping.hpp>

#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>

#include <sstream>

namespace launchdarkly {

tl::expected<enum EvaluationReason::Kind, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<enum EvaluationReason::Kind, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    if (!json_value.is_string()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    auto const& str = json_value.as_string();
    if (str == "OFF") {
        return EvaluationReason::Kind::kOff;
    }
    if (str == "FALLTHROUGH") {
        return EvaluationReason::Kind::kFallthrough;
    }
    if (str == "TARGET_MATCH") {
        return EvaluationReason::Kind::kTargetMatch;
    }
    if (str == "RULE_MATCH") {
        return EvaluationReason::Kind::kRuleMatch;
    }
    if (str == "PREREQUISITE_FAILED") {
        return EvaluationReason::Kind::kPrerequisiteFailed;
    }
    if (str == "ERROR") {
        return EvaluationReason::Kind::kError;
    }
    return tl::make_unexpected(JsonError::kSchemaFailure);
}

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                enum EvaluationReason::Kind const& kind) {
    boost::ignore_unused(unused);
    auto& str = json_value.emplace_string();
    switch (kind) {
        case EvaluationReason::Kind::kOff:
            str = "OFF";
            break;
        case EvaluationReason::Kind::kFallthrough:
            str = "FALLTHROUGH";
            break;
        case EvaluationReason::Kind::kTargetMatch:
            str = "TARGET_MATCH";
            break;
        case EvaluationReason::Kind::kRuleMatch:
            str = "RULE_MATCH";
            break;
        case EvaluationReason::Kind::kPrerequisiteFailed:
            str = "PREREQUISITE_FAILED";
            break;
        case EvaluationReason::Kind::kError:
            str = "ERROR";
            break;
    }
}

tl::expected<enum EvaluationReason::ErrorKind, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<enum EvaluationReason::ErrorKind,
                                           JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (!json_value.is_string()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    auto const& str = json_value.as_string();
    if (str == "CLIENT_NOT_READY") {
        return EvaluationReason::ErrorKind::kClientNotReady;
    }
    if (str == "USER_NOT_SPECIFIED") {
        return EvaluationReason::ErrorKind::kUserNotSpecified;
    }
    if (str == "FLAG_NOT_FOUND") {
        return EvaluationReason::ErrorKind::kFlagNotFound;
    }
    if (str == "WRONG_TYPE") {
        return EvaluationReason::ErrorKind::kWrongType;
    }
    if (str == "MALFORMED_FLAG") {
        return EvaluationReason::ErrorKind::kMalformedFlag;
    }
    if (str == "EXCEPTION") {
        return EvaluationReason::ErrorKind::kException;
    }
    return tl::make_unexpected(JsonError::kSchemaFailure);
}

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                enum EvaluationReason::ErrorKind const& kind) {
    boost::ignore_unused(unused);

    auto& str = json_value.emplace_string();
    switch (kind) {
        case EvaluationReason::ErrorKind::kClientNotReady:
            str = "CLIENT_NOT_READY";
            break;
        case EvaluationReason::ErrorKind::kUserNotSpecified:
            str = "USER_NOT_SPECIFIED";
            break;
        case EvaluationReason::ErrorKind::kFlagNotFound:
            str = "FLAG_NOT_FOUND";
            break;
        case EvaluationReason::ErrorKind::kWrongType:
            str = "WRONG_TYPE";
            break;
        case EvaluationReason::ErrorKind::kMalformedFlag:
            str = "MALFORMED_FLAG";
            break;
        case EvaluationReason::ErrorKind::kException:
            str = "EXCEPTION";
            break;
    }
}

tl::expected<enum EvaluationReason::BigSegmentsStatus, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<enum EvaluationReason::BigSegmentsStatus,
                     JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (!json_value.is_string()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    auto const& str = json_value.as_string();
    if (str == "HEALTHY") {
        return EvaluationReason::BigSegmentsStatus::kHealthy;
    }
    if (str == "STALE") {
        return EvaluationReason::BigSegmentsStatus::kStale;
    }
    if (str == "NOT_CONFIGURED") {
        return EvaluationReason::BigSegmentsStatus::kNotConfigured;
    }
    if (str == "STORE_ERROR") {
        return EvaluationReason::BigSegmentsStatus::kStoreError;
    }
    return tl::make_unexpected(JsonError::kSchemaFailure);
}

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                enum EvaluationReason::BigSegmentsStatus const& status) {
    boost::ignore_unused(unused);

    auto& str = json_value.emplace_string();
    switch (status) {
        case EvaluationReason::BigSegmentsStatus::kNone:
            // kNone is the "no status" sentinel and shouldn't reach the wire;
            // callers guard. If it ever does, emit an empty string.
            break;
        case EvaluationReason::BigSegmentsStatus::kHealthy:
            str = "HEALTHY";
            break;
        case EvaluationReason::BigSegmentsStatus::kStale:
            str = "STALE";
            break;
        case EvaluationReason::BigSegmentsStatus::kNotConfigured:
            str = "NOT_CONFIGURED";
            break;
        case EvaluationReason::BigSegmentsStatus::kStoreError:
            str = "STORE_ERROR";
            break;
    }
}

tl::expected<EvaluationReason, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<EvaluationReason, JsonError>> const&
        unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    if (json_value.is_object()) {
        auto& json_obj = json_value.as_object();

        auto kind_iter = json_obj.find("kind");
        if (kind_iter == json_obj.end()) {
            return tl::make_unexpected(JsonError::kSchemaFailure);
        }

        auto kind = boost::json::value_to<
            tl::expected<enum EvaluationReason::Kind, JsonError>>(
            kind_iter->value());

        if (!kind) {
            return tl::make_unexpected(kind.error());
        }

        auto* error_kind_iter = json_obj.find("errorKind");
        std::optional<enum EvaluationReason::ErrorKind> error_kind;
        if (error_kind_iter != json_obj.end()) {
            auto parsed = boost::json::value_to<
                tl::expected<enum EvaluationReason::ErrorKind, JsonError>>(
                error_kind_iter->value());
            if (!parsed) {
                return tl::make_unexpected(parsed.error());
            }
            error_kind = parsed.value();
        }

        auto* rule_index_iter = json_obj.find("ruleIndex");
        auto rule_index = ValueAsOpt<uint64_t>(rule_index_iter, json_obj.end());

        auto* rule_id_iter = json_obj.find("ruleId");
        auto rule_id = ValueAsOpt<std::string>(rule_id_iter, json_obj.end());

        auto* prerequisite_key_iter = json_obj.find("prerequisiteKey");
        auto prerequisite_key =
            ValueAsOpt<std::string>(prerequisite_key_iter, json_obj.end());

        auto* in_experiment_iter = json_obj.find("inExperiment");
        auto in_experiment =
            ValueOrDefault(in_experiment_iter, json_obj.end(), false);

        auto* big_segments_status_iter = json_obj.find("bigSegmentsStatus");
        auto big_segments_status = EvaluationReason::BigSegmentsStatus::kNone;
        if (big_segments_status_iter != json_obj.end()) {
            auto parsed = boost::json::value_to<tl::expected<
                enum EvaluationReason::BigSegmentsStatus, JsonError>>(
                big_segments_status_iter->value());
            if (!parsed) {
                return tl::make_unexpected(parsed.error());
            }
            big_segments_status = parsed.value();
        }

        return EvaluationReason{*kind,
                                error_kind,
                                rule_index,
                                rule_id,
                                prerequisite_key,
                                in_experiment,
                                big_segments_status};
    }
    return tl::unexpected(JsonError::kSchemaFailure);
}

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                EvaluationReason const& reason) {
    boost::ignore_unused(unused);

    auto& obj = json_value.emplace_object();
    obj.emplace("kind", boost::json::value_from(reason.Kind()));
    if (auto error_kind = reason.ErrorKind()) {
        obj.emplace("errorKind", boost::json::value_from(*error_kind));
    }
    auto big_segments_status = reason.BigSegmentsStatus();
    if (big_segments_status != EvaluationReason::BigSegmentsStatus::kNone) {
        obj.emplace("bigSegmentsStatus",
                    boost::json::value_from(big_segments_status));
    }
    if (auto rule_id = reason.RuleId()) {
        obj.emplace("ruleId", *rule_id);
    }
    if (auto rule_index = reason.RuleIndex()) {
        obj.emplace("ruleIndex", *rule_index);
    }
    if (reason.InExperiment()) {
        obj.emplace("inExperiment", true);
    }
    if (auto prereq_key = reason.PrerequisiteKey()) {
        obj.emplace("prerequisiteKey", *prereq_key);
    }
}
}  // namespace launchdarkly
