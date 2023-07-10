#include <utility>

#include "../boost_signal_connection.hpp"
#include "flag_updater.hpp"

namespace launchdarkly::client_side::flag_manager {

FlagUpdater::FlagUpdater(FlagStore& flag_store) : flag_store_(flag_store) {}

Value GetValue(ItemDescriptor& descriptor) {
    if (descriptor.item) {
        // `flag->` unwraps the first optional we know is present.
        // The second `value()` is not an optional.
        return descriptor.item->Detail().Value();
    }
    return {};
}

void FlagUpdater::Init(Context const& context,
                       std::unordered_map<std::string, ItemDescriptor> data) {
    std::lock_guard lock{signal_mutex_};

    // Calculate what flags changed.
    std::list<FlagValueChangeEvent> change_events;

    auto old_flags = flag_store_.GetAll();

    // No need to calculate any changes if nobody is listening to them.
    if (!old_flags.empty() && HasListeners()) {
        for (auto& new_pair : data) {
            auto existing = old_flags.find(new_pair.first);
            if (existing != old_flags.end()) {
                // The flag changed.
                auto& evaluation_result = new_pair.second.item;
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
                    // Deleted.
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

    flag_store_.Init(data);

    for (auto& event : change_events) {
        // Send the event.
        DispatchEvent(std::move(event));
    }
}
void FlagUpdater::DispatchEvent(FlagValueChangeEvent event) {
    auto handler = signals_.find(event.FlagName());
    if (handler != signals_.end()) {
        if (handler->second.empty()) {
            // Empty, remove it from the map, so it doesn't count toward
            // future calculations.
            signals_.erase(event.FlagName());
        } else {
            (handler->second)(
                std::make_shared<FlagValueChangeEvent>(std::move(event)));
        }
    }
}

void FlagUpdater::Upsert(Context const& context,
                         std::string key,
                         ItemDescriptor descriptor) {
    // Check the version.
    auto existing = flag_store_.Get(key);
    if (existing && (existing->version >= descriptor.version)) {
        // Out of order update, ignore it.
        return;
    }

    if (HasListeners()) {
        // Existed and updated.
        if (existing && descriptor.item) {
            DispatchEvent(FlagValueChangeEvent(key, GetValue(descriptor),
                                               GetValue(*existing)));
        } else if (descriptor.item) {
            DispatchEvent(FlagValueChangeEvent(
                key, descriptor.item.value().Detail().Value(), Value()));
            // new flag
        } else if (existing && existing->item.has_value()) {
            // Existed and deleted.
            DispatchEvent(FlagValueChangeEvent(key, GetValue(*existing)));
        } else {
            // Was deleted and is still deleted.
            // Do nothing.
        }
    }
    flag_store_.Upsert(key, descriptor);
}

bool FlagUpdater::HasListeners() const {
    std::lock_guard lock{signal_mutex_};
    return !signals_.empty();
}

std::unique_ptr<IConnection> FlagUpdater::OnFlagChange(
    std::string const& key,
    std::function<void(std::shared_ptr<FlagValueChangeEvent>)> handler) {
    std::lock_guard lock{signal_mutex_};
    return std::make_unique< ::launchdarkly::client_side::SignalConnection>(
        signals_[key].connect(handler));
}

}  // namespace launchdarkly::client_side::flag_manager
