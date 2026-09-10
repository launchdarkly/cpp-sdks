#include "flag_store.hpp"

#include <launchdarkly/encoding/sha_256.hpp>

#include <utility>

namespace launchdarkly::client_side::flag_manager {

// Shared pointers are used to item descriptors so that they may have a lifetime
// greater than their duration in the store. If, for instance, a flag has been
// accessed, and which it is being used init is called, then we want the
// flag being processed to be valid.

namespace {

// The evaluated value of a descriptor, or null if it has none.
Value FlagValue(ItemDescriptor const& descriptor) {
    if (descriptor.item) {
        return descriptor.item->Detail().Value();
    }
    return {};
}

}  // namespace

std::optional<FlagValueChangeEvent> ComputeFlagChange(
    std::string const& key,
    ItemDescriptor const* previous,
    ItemDescriptor const* current) {
    bool const had_value = previous && previous->item.has_value();
    bool const has_value = current && current->item.has_value();

    if (has_value) {
        auto new_value = FlagValue(*current);
        if (had_value) {
            auto old_value = FlagValue(*previous);
            if (new_value != old_value) {
                return FlagValueChangeEvent(key, std::move(new_value),
                                            std::move(old_value));
            }
            return std::nullopt;
        }
        return FlagValueChangeEvent(key, std::move(new_value), Value());
    }
    if (had_value) {
        return FlagValueChangeEvent(key, FlagValue(*previous));
    }
    return std::nullopt;
}

void FlagStore::Init(
    std::unordered_map<std::string, ItemDescriptor> const& data) {
    UpdateData(data);
}

void FlagStore::UpdateData(
    std::unordered_map<std::string, ItemDescriptor> const& data) {
    std::lock_guard lock{data_mutex_};
    this->data_.clear();
    for (auto item : data) {
        this->data_.emplace(item.first, std::make_shared<ItemDescriptor>(
                                            std::move(item.second)));
    }
}

void FlagStore::Upsert(std::string const& key, ItemDescriptor item) {
    std::lock_guard lock{data_mutex_};

    data_[key] = std::make_shared<ItemDescriptor>(std::move(item));
}

std::vector<FlagValueChangeEvent> FlagStore::Apply(
    FlagChangeSet const& change_set,
    bool compute_changes) {
    std::lock_guard lock{data_mutex_};

    std::vector<FlagValueChangeEvent> events;

    if (change_set.type == data_model::ChangeSetType::kNone) {
        return events;
    }

    bool const full = change_set.type == data_model::ChangeSetType::kFull;

    // Snapshotted so the events describe the transition the store actually
    // made.
    auto previous = std::move(data_);
    if (!full) {
        data_ = previous;
    } else {
        data_.clear();
    }

    // The first full data set is what the SDK starts from, not a change to
    // it, so it reports nothing.
    bool const report = compute_changes && !(full && previous.empty());

    for (auto const& change : change_set.data) {
        if (report) {
            auto const existing = previous.find(change.key);
            ItemDescriptor const* prev =
                existing != previous.end() ? existing->second.get() : nullptr;
            if (auto event = ComputeFlagChange(change.key, prev, &change.item)) {
                events.push_back(std::move(*event));
            }
        }

        data_[change.key] = std::make_shared<ItemDescriptor>(change.item);
    }

    // A full changeset is the complete data set, so anything it omits is gone.
    if (full && report) {
        for (auto const& [key, descriptor] : previous) {
            if (data_.count(key) == 0) {
                if (auto event =
                        ComputeFlagChange(key, descriptor.get(), nullptr)) {
                    events.push_back(std::move(*event));
                }
            }
        }
    }

    if (change_set.selector.value.has_value()) {
        selector_ = change_set.selector;
    } else {
        selector_ = data_model::Selector{};
    }

    return events;
}

data_model::Selector FlagStore::CurrentSelector() const {
    std::lock_guard lock{data_mutex_};

    return selector_;
}

void FlagStore::ClearSelector() {
    std::lock_guard lock{data_mutex_};

    selector_ = data_model::Selector{};
}

std::shared_ptr<ItemDescriptor> FlagStore::Get(
    std::string const& flag_key) const {
    std::lock_guard lock{data_mutex_};

    auto found = data_.find(flag_key);
    if (found != data_.end()) {
        return found->second;
    }
    return nullptr;
}

std::unordered_map<std::string, std::shared_ptr<ItemDescriptor>>
FlagStore::GetAll() const {
    std::lock_guard lock{data_mutex_};

    // Returns a copy of the map. (The descriptors are pointers and not shared).
    return data_;
}

}  // namespace launchdarkly::client_side::flag_manager
