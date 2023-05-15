#include "flag_manager.hpp"
#include "../data_sources/data_source_update_sink.hpp"

namespace launchdarkly::client_side::flag_manager {

// Shared pointers are used to item descriptors so that they may have a lifetime
// greater than their duration in the store. If, for instance, a flag has been
// accessed, and which it is being used init is called, then we want the
// flag being processed to be valid.

void FlagManager::Init(
    std::unordered_map<std::string, ItemDescriptor> const& data) {
    std::lock_guard lock{data_mutex_};

    data_.clear();
    for (auto item : data) {
        data_.emplace(item.first,
                      std::make_shared<ItemDescriptor>(std::move(item.second)));
    }
}

void FlagManager::Upsert(std::string const& key, ItemDescriptor item) {
    std::lock_guard lock{data_mutex_};

    data_[key] = std::make_shared<ItemDescriptor>(std::move(item));
}

std::shared_ptr<ItemDescriptor> FlagManager::Get(
    std::string const& flag_key) const {
    std::lock_guard lock{data_mutex_};

    auto found = data_.find(flag_key);
    if (found != data_.end()) {
        return found->second;
    }
    return nullptr;
}

std::unordered_map<std::string, std::shared_ptr<ItemDescriptor>>
FlagManager::GetAll() const {
    std::lock_guard lock{data_mutex_};

    // Returns a copy of the map. (The descriptors are pointers and not shared).
    return data_;
}

}  // namespace launchdarkly::client_side::flag_manager
