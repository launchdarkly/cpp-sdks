#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>
#include <launchdarkly/serialization/json_fdv2_events.hpp>
#include <launchdarkly/serialization/value_mapping.hpp>
#include <tl/expected.hpp>

namespace launchdarkly {

tl::expected<std::optional<IntentCode>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<IntentCode>, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_STRING(json_value);

    auto const& str = json_value.as_string();
    if (str == "none") {
        return IntentCode::kNone;
    } else if (str == "xfer-full") {
        return IntentCode::kTransferFull;
    } else if (str == "xfer-changes") {
        return IntentCode::kTransferChanges;
    } else {
        return IntentCode::kUnknown;
    }
}

tl::expected<std::optional<ServerIntentPayload>, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<std::optional<ServerIntentPayload>,
                                           JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    ServerIntentPayload payload{};

    PARSE_REQUIRED_FIELD(payload.id, obj, "id");
    PARSE_REQUIRED_FIELD(payload.target, obj, "target");
    PARSE_REQUIRED_FIELD(payload.intent_code, obj, "intentCode");
    PARSE_CONDITIONAL_FIELD(payload.reason, obj, "reason");

    return payload;
}

tl::expected<std::optional<ServerIntent>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<ServerIntent>, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    ServerIntent intent{};

    PARSE_REQUIRED_FIELD(intent.payloads, obj, "payloads");

    return intent;
}

tl::expected<std::optional<PutObject>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<PutObject>, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    PutObject put{};

    PARSE_REQUIRED_FIELD(put.version, obj, "version");
    PARSE_REQUIRED_FIELD(put.kind, obj, "kind");
    PARSE_REQUIRED_FIELD(put.key, obj, "key");

    auto const& it = obj.find("object");
    if (it == obj.end()) {
        return tl::make_unexpected(JsonError::kSchemaFailure);
    }
    put.object = it->value();

    return put;
}

tl::expected<std::optional<DeleteObject>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<DeleteObject>, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    DeleteObject del{};

    PARSE_REQUIRED_FIELD(del.version, obj, "version");
    PARSE_REQUIRED_FIELD(del.kind, obj, "kind");
    PARSE_REQUIRED_FIELD(del.key, obj, "key");

    return del;
}

tl::expected<std::optional<PayloadTransferred>, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<std::optional<PayloadTransferred>,
                                           JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    PayloadTransferred transferred{};

    PARSE_REQUIRED_FIELD(transferred.state, obj, "state");
    PARSE_REQUIRED_FIELD(transferred.version, obj, "version");

    return transferred;
}

tl::expected<std::optional<Goodbye>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<Goodbye>, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    Goodbye goodbye{};

    PARSE_CONDITIONAL_FIELD(goodbye.reason, obj, "reason");

    return goodbye;
}

tl::expected<std::optional<FDv2Error>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<FDv2Error>, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    REQUIRE_OBJECT(json_value);
    auto const& obj = json_value.as_object();

    FDv2Error error{};

    PARSE_REQUIRED_FIELD(error.reason, obj, "reason");
    PARSE_CONDITIONAL_FIELD(error.id, obj, "id");

    return error;
}

}  // namespace launchdarkly
