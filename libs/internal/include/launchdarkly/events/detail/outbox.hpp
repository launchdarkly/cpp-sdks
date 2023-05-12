#pragma once

#include <launchdarkly/events/events.hpp>
#include <mutex>
#include <queue>
#include <string>

namespace launchdarkly::events::detail {

/**
 * Represents a fixed-size queue for holding output events, which are events
 * ready for JSON serialization (followed by delivery to LaunchDarkly).
 */
class Outbox {
   public:
    /**
     * Constructs an Outbox with the given capacity.
     * @param capacity Capacity after which pushed events will be discarded.
     */
    explicit Outbox(std::size_t capacity);

    /**
     * Pushes a collection of events into the outbox. Events will be
     * individually pushed one-by-one until there are no more events or the
     * outbox is full; additional events will be dropped.
     * @param events Events to push.
     * @return True if all events were accepted; false if >= 1 events were
     * dropped.
     */
    bool PushDiscardingOverflow(std::vector<OutputEvent> events);

    /**
     * Consumes all events in the outbox.
     * @return All events in the outbox, in the order they were pushed.
     */
    std::vector<OutputEvent> Consume();

    /**
     * True if the outbox is empty.
     */
    bool Empty();

   private:
    std::queue<OutputEvent> items_;
    std::size_t capacity_;

    // Pushes an item, returning true if accepted (otherwise drops the event
    // and returns false.)
    bool Push(OutputEvent item);
};

}  // namespace launchdarkly::events::detail
