#include <utility>

#include <launchdarkly/signals/boost_signal_connection.hpp>

#include "flag_updater.hpp"

namespace launchdarkly::client_side::flag_manager {

FlagUpdater::FlagUpdater(FlagStore& flag_store) : flag_store_(flag_store) {}

void FlagUpdater::Init(Context const& context,
                       std::unordered_map<std::string, ItemDescriptor> data) {
    std::lock_guard lock{signal_mutex_};

    std::list<FlagValueChangeEvent> change_events;

    auto old_flags = flag_store_.GetAll();

    // No need to calculate any changes if nobody is listening to them.
    if (!old_flags.empty() && HasListeners()) {
        for (auto& new_pair : data) {
            auto existing = old_flags.find(new_pair.first);
            ItemDescriptor const* previous =
                existing != old_flags.end() ? existing->second.get() : nullptr;
            if (auto event = ComputeFlagChange(new_pair.first, previous,
                                               &new_pair.second)) {
                change_events.push_back(std::move(*event));
            }
        }
        for (auto& old_pair : old_flags) {
            if (data.count(old_pair.first) == 0) {
                if (auto event = ComputeFlagChange(
                        old_pair.first, old_pair.second.get(), nullptr)) {
                    change_events.push_back(std::move(*event));
                }
            }
        }
    }

    flag_store_.Init(data);

    for (auto& event : change_events) {
        DispatchEvent(std::move(event));
    }
}
void FlagUpdater::Apply(Context const& context,
                        FlagChangeSet change_set,
                        bool /* from_cache */) {
    std::lock_guard lock{signal_mutex_};

    auto events = flag_store_.Apply(change_set, HasListeners());

    for (auto& event : events) {
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

    flag_store_.Upsert(key, descriptor);
    if (HasListeners()) {
        if (auto event = ComputeFlagChange(key, existing.get(), &descriptor)) {
            DispatchEvent(std::move(*event));
        }
    }
}

bool FlagUpdater::HasListeners() const {
    std::lock_guard lock{signal_mutex_};
    return !signals_.empty();
}

std::unique_ptr<IConnection> FlagUpdater::OnFlagChange(
    std::string const& key,
    std::function<void(std::shared_ptr<FlagValueChangeEvent>)> handler) {
    std::lock_guard lock{signal_mutex_};
    return std::make_unique<launchdarkly::internal::signals::SignalConnection>(
        signals_[key].connect(handler));
}

}  // namespace launchdarkly::client_side::flag_manager
