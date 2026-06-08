#pragma once

#include <launchdarkly/data_model/fdv2_change.hpp>
#include <launchdarkly/serialization/json_fdv2_events.hpp>

#include <boost/json/value.hpp>

#include <string_view>
#include <variant>
#include <vector>

namespace launchdarkly {

/**
 * Protocol state machine for the FDv2 wire format.
 *
 * A server-intent opens a transfer cycle: put-object and delete-object
 * events accumulate until a payload-transferred event, which emits an
 * FDv2ChangeSet. The handler then remains active — subsequent put/delete
 * + payload-transferred cycles emit kPartial changesets reusing the prior
 * intent, until the server sends a new server-intent, error, or goodbye.
 *
 * Shared between the polling and streaming synchronizers.
 */
class FDv2ProtocolHandler {
   public:
    /**
     * Typed error returned by HandleEvent. Carries the original underlying
     * error context rather than converting to a plain string.
     */
    struct Error {
        enum class Kind {
            kJsonError,      // Failed to deserialise an event's data field.
            kProtocolError,  // Out-of-order or unexpected event.
            kServerError,    // Server sent a valid 'error' event.
        };

        Kind kind;
        std::string message;

        /**
         * Set for kJsonError when the tl::expected parse returned an error.
         * Nullopt when parse succeeded but the data value was null.
         */
        std::optional<JsonError> json_error;

        /**
         * Set for kServerError: the full wire error including id and reason.
         */
        std::optional<FDv2Error> server_error;

        /** JSON deserialisation failed — carries the original JsonError. */
        static Error JsonParseError(JsonError err, std::string msg) {
            return {Kind::kJsonError, std::move(msg), err, std::nullopt};
        }
        /** Parse succeeded but data was null — no underlying JsonError. */
        static Error JsonParseError(std::string msg) {
            return {Kind::kJsonError, std::move(msg), std::nullopt,
                    std::nullopt};
        }
        /** Out-of-order or unexpected protocol event. */
        static Error ProtocolError(std::string msg) {
            return {Kind::kProtocolError, std::move(msg), std::nullopt,
                    std::nullopt};
        }
        /** Server sent a well-formed 'error' event. */
        static Error ServerError(FDv2Error err) {
            return {Kind::kServerError, err.reason, std::nullopt,
                    std::move(err)};
        }
    };

    /**
     * Result of handling a single FDv2 event:
     * - monostate: no output yet (accumulating, heartbeat, or unknown event)
     * - FDv2ChangeSet: complete changeset ready to apply
     * - Error: protocol error (JSON parse failure, protocol violation, or
     *          server-sent error event)
     * - Goodbye: server is closing; caller should rotate sources
     */
    using Result =
        std::variant<std::monostate, data_model::FDv2ChangeSet, Error, Goodbye>;

    /**
     * Process one FDv2 event.
     *
     * @param event_type The event type string (e.g. "server-intent",
     *                   "put-object", "payload-transferred").
     * @param data       The parsed JSON value for the event's data field.
     * @return           A Result indicating what (if anything) the caller
     *                   should act on.
     */
    Result HandleEvent(std::string_view event_type,
                       boost::json::value const& data);

    /**
     * Reset accumulated state. Call on reconnect before processing new events.
     */
    void Reset();

    /**
     * @return true if event_type is one that the protocol handler recognizes
     * and may dispatch on. Events outside this set are spec-defined as
     * "unrecognized data that can be safely ignored".
     */
    static bool IsKnownEvent(std::string_view event_type);

    FDv2ProtocolHandler() = default;

   private:
    enum class State { kInactive, kFull, kPartial };

    Result HandleServerIntent(boost::json::value const& data);
    Result HandlePutObject(boost::json::value const& data);
    Result HandleDeleteObject(boost::json::value const& data);
    Result HandlePayloadTransferred(boost::json::value const& data);
    Result HandleError(boost::json::value const& data);
    Result HandleGoodbye(boost::json::value const& data);

    State state_ = State::kInactive;
    std::vector<data_model::FDv2Change> changes_;
};

}  // namespace launchdarkly
