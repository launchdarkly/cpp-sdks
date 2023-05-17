#include <boost/json.hpp>

#include "../encoding/sha_256.hpp"
#include "../serialization/json_all_flags.hpp"
#include "context_index.hpp"
#include "flag_store.hpp"

#include <boost/json/parse.hpp>
#include <boost/json/system_error.hpp>
#include <launchdarkly/serialization/json_evaluation_result.hpp>

namespace launchdarkly::client_side::flag_manager {

// Shared pointers are used to item descriptors so that they may have a lifetime
// greater than their duration in the store. If, for instance, a flag has been
// accessed, and which it is being used init is called, then we want the
// flag being processed to be valid.

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
