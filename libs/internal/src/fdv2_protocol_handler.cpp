#include <launchdarkly/fdv2_protocol_handler.hpp>

#include <boost/json.hpp>
#include <tl/expected.hpp>

namespace launchdarkly {

static char const* const kServerIntent = "server-intent";
static char const* const kPutObject = "put-object";
static char const* const kDeleteObject = "delete-object";
static char const* const kPayloadTransferred = "payload-transferred";
static char const* const kError = "error";
static char const* const kGoodbye = "goodbye";

using Error = FDv2ProtocolHandler::Error;

FDv2ProtocolHandler::Result FDv2ProtocolHandler::HandleEvent(
    std::string_view event_type,
    boost::json::value const& data) {
    if (event_type == kServerIntent) {
        return HandleServerIntent(data);
    }
    if (event_type == kPutObject) {
        return HandlePutObject(data);
    }
    if (event_type == kDeleteObject) {
        return HandleDeleteObject(data);
    }
    if (event_type == kPayloadTransferred) {
        return HandlePayloadTransferred(data);
    }
    if (event_type == kError) {
        return HandleError(data);
    }
    if (event_type == kGoodbye) {
        return HandleGoodbye(data);
    }
    // heartbeat and unrecognized events: no-op.
    return std::monostate{};
}

FDv2ProtocolHandler::Result FDv2ProtocolHandler::HandleServerIntent(
    boost::json::value const& data) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<ServerIntent>, JsonError>>(data);
    if (!result) {
        Reset();
        return Error::JsonParseError(result.error(),
                                     "could not deserialize server-intent");
    }
    if (!result->has_value()) {
        Reset();
        return Error::JsonParseError("server-intent data was null");
    }
    auto const& intent = **result;
    if (intent.payloads.empty()) {
        // The protocol requires exactly one payload per server-intent, so
        // an empty payloads array is a spec violation. Reset to avoid
        // leaking accumulated state from a prior incomplete transfer.
        Reset();
        return std::monostate{};
    }
    // The protocol defines exactly one payload per intent.
    auto const& code = intent.payloads[0].intent_code;
    changes_.clear();
    if (code == IntentCode::kTransferFull) {
        state_ = State::kFull;
    } else if (code == IntentCode::kTransferChanges) {
        state_ = State::kPartial;
    } else {
        // kNone or kUnknown: emit an empty changeset immediately.
        state_ = State::kInactive;
        return data_model::FDv2ChangeSet{
            data_model::ChangeSetType::kNone, {}, data_model::Selector{}};
    }
    return std::monostate{};
}

FDv2ProtocolHandler::Result FDv2ProtocolHandler::HandlePutObject(
    boost::json::value const& data) {
    if (state_ == State::kInactive) {
        return std::monostate{};
    }
    auto result = boost::json::value_to<
        tl::expected<std::optional<PutObject>, JsonError>>(data);
    if (!result) {
        Reset();
        return Error::JsonParseError(result.error(),
                                     "could not deserialize put-object");
    }
    if (!result->has_value()) {
        Reset();
        return Error::JsonParseError("put-object data was null");
    }
    auto const& put = **result;
    changes_.push_back(data_model::FDv2Change{
        data_model::FDv2Change::ChangeType::kPut, put.kind, put.key,
        static_cast<uint64_t>(put.version), put.object});
    return std::monostate{};
}

FDv2ProtocolHandler::Result FDv2ProtocolHandler::HandleDeleteObject(
    boost::json::value const& data) {
    if (state_ == State::kInactive) {
        return std::monostate{};
    }
    auto result = boost::json::value_to<
        tl::expected<std::optional<DeleteObject>, JsonError>>(data);
    if (!result) {
        Reset();
        return Error::JsonParseError(result.error(),
                                     "could not deserialize delete-object");
    }
    if (!result->has_value()) {
        Reset();
        return Error::JsonParseError("delete-object data was null");
    }
    auto const& del = **result;
    changes_.push_back(
        data_model::FDv2Change{data_model::FDv2Change::ChangeType::kDelete,
                               del.kind,
                               del.key,
                               static_cast<uint64_t>(del.version),
                               {}});
    return std::monostate{};
}

FDv2ProtocolHandler::Result FDv2ProtocolHandler::HandlePayloadTransferred(
    boost::json::value const& data) {
    if (state_ == State::kInactive) {
        Reset();
        return Error::ProtocolError(
            "payload-transferred received without an active "
            "server-intent");
    }
    auto result = boost::json::value_to<
        tl::expected<std::optional<PayloadTransferred>, JsonError>>(data);
    if (!result) {
        Reset();
        return Error::JsonParseError(
            result.error(), "could not deserialize payload-transferred");
    }
    if (!result->has_value()) {
        Reset();
        return Error::JsonParseError("payload-transferred data was null");
    }
    auto const& transferred = **result;
    auto type = (state_ == State::kPartial)
                    ? data_model::ChangeSetType::kPartial
                    : data_model::ChangeSetType::kFull;
    data_model::FDv2ChangeSet changeset{
        type, std::move(changes_),
        data_model::Selector{data_model::Selector::State{transferred.version,
                                                         transferred.state}}};
    Reset();
    return changeset;
}

FDv2ProtocolHandler::Result FDv2ProtocolHandler::HandleError(
    boost::json::value const& data) {
    auto result = boost::json::value_to<
        tl::expected<std::optional<FDv2Error>, JsonError>>(data);
    Reset();
    if (!result) {
        return Error::JsonParseError(result.error(),
                                     "could not deserialize error event");
    }
    if (!result->has_value()) {
        return Error::JsonParseError("error event data was null");
    }
    return Error::ServerError(std::move(**result));
}

FDv2ProtocolHandler::Result FDv2ProtocolHandler::HandleGoodbye(
    boost::json::value const& data) {
    Reset();
    auto result =
        boost::json::value_to<tl::expected<std::optional<Goodbye>, JsonError>>(
            data);
    // Parse failures are intentionally ignored: the caller should rotate
    // sources regardless of whether the reason field is readable.
    if (!result) {
        return Goodbye{std::nullopt};
    }
    if (!result->has_value()) {
        return Goodbye{std::nullopt};
    }
    return **result;
}

void FDv2ProtocolHandler::Reset() {
    state_ = State::kInactive;
    changes_.clear();
}

}  // namespace launchdarkly
