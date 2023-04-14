#include "launchdarkly/client_side/flag_manager/detail/flag_updater.hpp"

namespace launchdarkly::client_side::flag_manager::detail {

FlagUpdater::FlagUpdater(FlagManager& flag_manager)
    : flag_manager_(flag_manager) {}

Value GetValue(ItemDescriptor& descriptor) {
    if (descriptor.flag) {
        // The First `value()` is an optional, we already know
        // it is present. The second `value()` is not an
        // optional.
        return descriptor.flag.value().detail().value();
    }
    return Value();
}

void FlagUpdater::init(std::unordered_map<std::string, ItemDescriptor> data) {
    std::lock_guard{signal_mutex_};

    // Calculate what flags changed.
    std::list<FlagValueChangeEvent> change_events;

    auto old_flags = flag_manager_.get_all();

    // No need to calculate any changes if nobody is listening to them.
    if (old_flags.size() && has_listeners()) {
        // TODO: Should we send events for ALL of the flags when they
        // are first added?
        for (auto& new_pair : data) {
            auto existing = old_flags.find(new_pair.first);
            if (existing != old_flags.end()) {
                // The flag changed.
                auto& evaluation_result = new_pair.second.flag;
                if (evaluation_result) {
                    auto new_value = GetValue(new_pair.second);
                    auto old_value = GetValue(*existing->second);
                    if (new_value != old_value) {
                        // Updated.
                        change_events.emplace_back(new_pair.first,
                                                   GetValue(new_pair.second),
                                                   GetValue(*existing->second));
                    }
                } else {
                    change_events.emplace_back(existing->first,
                                               GetValue(*existing->second));
                }

            } else {
                // It is a new flag.
                change_events.emplace_back(new_pair.first,
                                           GetValue(new_pair.second), Value());
            }
        }
        for (auto& old_pair : old_flags) {
            auto still_exists = data.count(old_pair.first) != 0;
            if (!still_exists) {
                // Was in the old data, but not the new data, so it was deleted.
                change_events.emplace_back(old_pair.first,
                                           GetValue(*old_pair.second));
            }
        }
    }

    flag_manager_.init(data);

    for (auto& event : change_events) {
        // Send the event.
        dispatch_event(std::move(event));
    }
}
void FlagUpdater::dispatch_event(FlagValueChangeEvent event) {
    auto handler = signals_.find(event.flag_name());
    if (handler != signals_.end()) {
        if (handler->second.empty()) {
            // Empty, remove it from the map so it doesn't count toward
            // future calculations.
            signals_.erase(event.flag_name());
        } else {
            (handler->second)(
                std::make_shared<FlagValueChangeEvent>(std::move(event)));
        }
    }
}

void FlagUpdater::upsert(std::string key, ItemDescriptor item) {
    // Check the version.
    auto existing = flag_manager_.get(key);
    if (existing && (existing->version > item.version)) {
        // Out of order update, ignore it.
        return;
    }

    // Existed and updated.
    if (existing && item.flag) {
        dispatch_event(
            FlagValueChangeEvent(key, GetValue(item), GetValue(*existing)));
    } else if (item.flag) {
        dispatch_event(
            FlagValueChangeEvent(key, item.flag.value().detail().value()));
        // new flag
    } else if (existing) {
        // Existed and deleted.
        dispatch_event(FlagValueChangeEvent(key, Value()));
    } else {
        // Was deleted and is still deleted.
        // Do nothing.
    }
    flag_manager_.upsert(key, item);
}

bool FlagUpdater::has_listeners() const {
    std::lock_guard{signal_mutex_};
    if (signals_.empty()) {
        return false;
    }
    return true;
}

}  // namespace launchdarkly::client_side::flag_manager::detail
