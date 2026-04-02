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
 * Accumulates put-object and delete-object events between a server-intent
 * and payload-transferred event, then emits a complete FDv2ChangeSet.
 *
 * Shared between the polling and streaming synchronizers.
 */
class FDv2ProtocolHandler {
   public:
    /**
     * Result of handling a single FDv2 event:
     * - monostate: no output yet (accumulating, heartbeat, or unknown event)
     * - FDv2ChangeSet: complete changeset ready to apply
     * - FDv2Error: server reported an error; discard partial data
     * - Goodbye: server is closing; caller should rotate sources
     */
    using Result = std::variant<std::monostate,
                                data_model::FDv2ChangeSet,
                                FDv2Error,
                                Goodbye>;

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

    FDv2ProtocolHandler() = default;

   private:
    enum class State { kInactive, kFull, kPartial };

    State state_ = State::kInactive;
    std::vector<data_model::FDv2Change> changes_;
};

}  // namespace launchdarkly
