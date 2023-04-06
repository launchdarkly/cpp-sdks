#include "events/detail/outbox.hpp"

namespace launchdarkly::events::detail {

Outbox::Outbox(std::size_t capacity) : items_(), capacity_(capacity) {}

bool Outbox::push(OutputEvent item) {
    if (items_.size() >= capacity_) {
        return false;
    }
    items_.push(std::move(item));
    return true;
}

bool Outbox::push_discard_overflow(std::vector<OutputEvent> items) {
    auto begin = std::make_move_iterator(items.begin());
    auto end = std::make_move_iterator(items.end());

    for (auto it = begin; it != end; it++) {
        if (!push(std::move(*it))) {
            return false;
        }
    }
    return true;
}

std::vector<OutputEvent> Outbox::consume() {
    std::vector<OutputEvent> out;
    out.reserve(items_.size());
    while (!items_.empty()) {
        out.push_back(std::move(items_.front()));
        items_.pop();
    }
    return out;
}

bool Outbox::empty() {
    return items_.empty();
}

std::size_t Outbox::capacity() {
    return capacity_;
}

bool Outbox::full() {
    return items_.size() == capacity_;
}

void Outbox::clear() {
    while (!items_.empty()) {
        items_.pop();
    }
}

}  // namespace launchdarkly::events::detail
