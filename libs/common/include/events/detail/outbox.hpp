#pragma once

#include <mutex>
#include <queue>
#include <string>
#include "events/events.hpp"

namespace launchdarkly::events::detail {

class Outbox {
   public:
    Outbox(std::size_t capacity);
    bool push(OutputEvent item);
    bool push_discard_overflow(std::vector<OutputEvent> items);

    std::vector<OutputEvent> consume();
    bool empty();
    bool full();

    void clear();

    std::size_t capacity();

   private:
    std::queue<OutputEvent> items_;
    std::size_t capacity_;
};

}  // namespace launchdarkly::events::detail
