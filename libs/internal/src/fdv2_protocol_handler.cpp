#include <launchdarkly/fdv2_protocol_handler.hpp>

#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/item_descriptor.hpp>
#include <launchdarkly/data_model/segment.hpp>
#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

#include <boost/json.hpp>
#include <tl/expected.hpp>

namespace launchdarkly {

static char const* const kServerIntent = "server-intent";
static char const* const kPutObject = "put-object";
static char const* const kDeleteObject = "delete-object";
static char const* const kPayloadTransferred = "payload-transferred";
static char const* const kError = "error";
static char const* const kGoodbye = "goodbye";

// Returns the parsed FDv2Change on success, nullopt for unknown kinds (which
// should be silently skipped for forward-compatibility), or an error string if
// a known kind fails to deserialize.
static tl::expected<std::optional<data_model::FDv2Change>, std::string>
ParsePut(PutObject const& put) {
    if (put.kind == "flag") {
        auto result = boost::json::value_to<
            tl::expected<std::optional<data_model::Flag>, JsonError>>(
            put.object);
        // One bad flag aborts the entire transfer so the store is never
        // left in a partially-updated state.
        if (!result) {
            return tl::make_unexpected("could not deserialize flag '" +
                                       put.key + "'");
        }
        if (!result->has_value()) {
            return tl::make_unexpected("flag '" + put.key + "' object was null");
        }
        return data_model::FDv2Change{
            put.key,
            data_model::ItemDescriptor<data_model::Flag>{std::move(**result)}};
    }
    if (put.kind == "segment") {
        auto result = boost::json::value_to<
            tl::expected<std::optional<data_model::Segment>, JsonError>>(
            put.object);
        // One bad segment aborts the entire transfer so the store is never
        // left in a partially-updated state.
        if (!result) {
            return tl::make_unexpected("could not deserialize segment '" +
                                       put.key + "'");
        }
        if (!result->has_value()) {
            return tl::make_unexpected("segment '" + put.key +
                                       "' object was null");
        }
        return data_model::FDv2Change{
            put.key,
            data_model::ItemDescriptor<data_model::Segment>{
                std::move(**result)}};
    }
    // Silently skip unknown kinds for forward-compatibility.
    return std::nullopt;
}

static data_model::FDv2Change MakeDeleteChange(DeleteObject const& del) {
    if (del.kind == "flag") {
        return data_model::FDv2Change{
            del.key,
            data_model::ItemDescriptor<data_model::Flag>{
                data_model::Tombstone{static_cast<uint64_t>(del.version)}}};
    }
    return data_model::FDv2Change{
        del.key,
        data_model::ItemDescriptor<data_model::Segment>{
            data_model::Tombstone{static_cast<uint64_t>(del.version)}}};
}

FDv2ProtocolHandler::Result FDv2ProtocolHandler::HandleEvent(
    std::string_view event_type,
    boost::json::value const& data) {
    if (event_type == kServerIntent) {
        auto result = boost::json::value_to<
            tl::expected<std::optional<ServerIntent>, JsonError>>(data);
        if (!result) {
            Reset();
            return FDv2Error{std::nullopt, "could not deserialize server-intent"};
        }
        if (!result->has_value()) {
            Reset();
            return FDv2Error{std::nullopt, "server-intent data was null"};
        }
        auto const& intent = **result;
        if (intent.payloads.empty()) {
            return std::monostate{};
        }
        auto const& code = intent.payloads[0].intent_code;
        changes_.clear();
        if (code == IntentCode::kTransferFull) {
            state_ = State::kFull;
        } else if (code == IntentCode::kTransferChanges) {
            state_ = State::kPartial;
        } else {
            // kNone or kUnknown: emit an empty changeset immediately.
            state_ = State::kInactive;
            return data_model::FDv2ChangeSet{data_model::FDv2ChangeSet::Type::kNone,
                                            {},
                                            data_model::Selector{}};
        }
        return std::monostate{};
    }

    if (event_type == kPutObject) {
        if (state_ == State::kInactive) {
            return std::monostate{};
        }
        auto result = boost::json::value_to<
            tl::expected<std::optional<PutObject>, JsonError>>(data);
        if (!result) {
            Reset();
            return FDv2Error{std::nullopt, "could not deserialize put-object"};
        }
        if (!result->has_value()) {
            Reset();
            return FDv2Error{std::nullopt, "put-object data was null"};
        }
        auto change = ParsePut(**result);
        if (!change) {
            Reset();
            return FDv2Error{std::nullopt, std::move(change.error())};
        }
        if (*change) {
            changes_.push_back(std::move(**change));
        }
        return std::monostate{};
    }

    if (event_type == kDeleteObject) {
        if (state_ == State::kInactive) {
            return std::monostate{};
        }
        auto result = boost::json::value_to<
            tl::expected<std::optional<DeleteObject>, JsonError>>(data);
        if (!result) {
            Reset();
            return FDv2Error{std::nullopt, "could not deserialize delete-object"};
        }
        if (!result->has_value()) {
            Reset();
            return FDv2Error{std::nullopt, "delete-object data was null"};
        }
        auto const& del = **result;
        // Silently skip unknown kinds for forward-compatibility.
        if (del.kind != "flag" && del.kind != "segment") {
            return std::monostate{};
        }
        changes_.push_back(MakeDeleteChange(del));
        return std::monostate{};
    }

    if (event_type == kPayloadTransferred) {
        if (state_ == State::kInactive) {
            Reset();
            return FDv2Error{std::nullopt,
                             "payload-transferred received without an active "
                             "server-intent"};
        }
        auto result = boost::json::value_to<
            tl::expected<std::optional<PayloadTransferred>, JsonError>>(data);
        if (!result) {
            Reset();
            return FDv2Error{std::nullopt,
                             "could not deserialize payload-transferred"};
        }
        if (!result->has_value()) {
            Reset();
            return FDv2Error{std::nullopt, "payload-transferred data was null"};
        }
        auto const& transferred = **result;
        auto type = (state_ == State::kPartial)
                        ? data_model::FDv2ChangeSet::Type::kPartial
                        : data_model::FDv2ChangeSet::Type::kFull;
        data_model::FDv2ChangeSet changeset{
            type,
            std::move(changes_),
            data_model::Selector{data_model::Selector::State{
                transferred.version, transferred.state}}};
        Reset();
        return changeset;
    }

    if (event_type == kError) {
        auto result = boost::json::value_to<
            tl::expected<std::optional<FDv2Error>, JsonError>>(data);
        Reset();
        if (!result) {
            return FDv2Error{std::nullopt, "could not deserialize error event"};
        }
        if (!result->has_value()) {
            return FDv2Error{std::nullopt, "error event data was null"};
        }
        return **result;
    }

    if (event_type == kGoodbye) {
        auto result = boost::json::value_to<
            tl::expected<std::optional<Goodbye>, JsonError>>(data);
        if (!result) {
            return Goodbye{std::nullopt};
        }
        if (!result->has_value()) {
            return Goodbye{std::nullopt};
        }
        return **result;
    }

    // heartbeat and unrecognized events: no-op.
    return std::monostate{};
}

void FDv2ProtocolHandler::Reset() {
    state_ = State::kInactive;
    changes_.clear();
}

}  // namespace launchdarkly
