#include "launchdarkly/events/detail/outbox.hpp"

namespace launchdarkly::events {

Outbox::Outbox(std::size_t capacity) : items_(), capacity_(capacity) {}

bool Outbox::Push(OutputEvent item) {
    if (items_.size() >= capacity_) {
        return false;
    }
    items_.push(std::move(item));
    return true;
}

bool Outbox::PushDiscardingOverflow(std::vector<OutputEvent> items) {
    auto begin = std::make_move_iterator(items.begin());
    auto end = std::make_move_iterator(items.end());

    for (auto it = begin; it != end; it++) {
        if (!Push(std::move(*it))) {
            return false;
        }
    }
    return true;
}

std::vector<OutputEvent> Outbox::Consume() {
    std::vector<OutputEvent> out;
    out.reserve(items_.size());
    while (!items_.empty()) {
        out.push_back(std::move(items_.front()));
        items_.pop();
    }
    return out;
}

bool Outbox::Empty() {
    return items_.empty();
}

}  // namespace launchdarkly::events
