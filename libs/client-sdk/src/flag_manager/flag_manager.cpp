#include <boost/json.hpp>

#include "../persistence/context_index.hpp"
#include "../serialization/json_all_flags.hpp"
#include "flag_manager.hpp"

#include <boost/json/parse.hpp>
#include <boost/json/system_error.hpp>
#include <launchdarkly/serialization/json_evaluation_result.hpp>

namespace launchdarkly::client_side::flag_manager {

// Shared pointers are used to item descriptors so that they may have a lifetime
// greater than their duration in the store. If, for instance, a flag has been
// accessed, and which it is being used init is called, then we want the
// flag being processed to be valid.

FlagManager::FlagManager(std::shared_ptr<IPersistence> persistence,
                         std::string environment_namespace)
    : persistence_(persistence),
      environment_namespace_(std::move(environment_namespace)) {}

void FlagManager::Init(
    std::unordered_map<std::string, ItemDescriptor> const& data) {
    UpdateData(data);
}

void FlagManager::UpdateData(
    std::unordered_map<std::string, ItemDescriptor> const& data) {
    std::lock_guard lock{data_mutex_};
    this->data_.clear();
    for (auto item : data) {
        this->data_.emplace(item.first, std::make_shared<ItemDescriptor>(
                                            std::move(item.second)));
    }
}

void FlagManager::Upsert(std::string const& key, ItemDescriptor item) {
    std::lock_guard lock{data_mutex_};

    data_[key] = std::make_shared<ItemDescriptor>(std::move(item));
}

void FlagManager::LoadCache(Context const& context) {
    if (persistence_) {
        std::lock_guard lock(persistence_mutex_);
        // TODO: Hash the canonical key.
        auto data =
            persistence_->Read(environment_namespace_, context.canonical_key());
        if (data) {
            boost::json::error_code error_code;
            auto parsed = boost::json::parse(*data, error_code);
            if (error_code) {
                // TODO: Log?
            } else {
                auto res = boost::json::value_to<tl::expected<
                    std::unordered_map<
                        std::string, launchdarkly::client_side::ItemDescriptor>,
                    JsonError>>(parsed);
                if (res) {
                    UpdateData(*res);
                } else {
                    // TODO: Log?
                }
            }
        }
    }
}

void FlagManager::StoreCache(std::string context_id) {
    if (persistence_) {
        std::lock_guard lock(persistence_mutex_);
        auto index = GetIndex();
        index.Notice(context_id);
        auto pruned = index.Prune(max_cached_contexts_);
        for (auto& id : pruned) {
            persistence_->RemoveValue(environment_namespace_, id);
        }
        persistence_->SetValue(
            global_namespace_, index_key_,
            boost::json::serialize(boost::json::value_from(index)));

        persistence_->SetValue(
            environment_namespace_, context_id,
            boost::json::serialize(boost::json::value_from(GetAll())));
    }
}

persistence::ContextIndex FlagManager::GetIndex() {
    if (persistence_) {
        std::lock_guard lock(persistence_mutex_);
        auto index_data = persistence_->Read(global_namespace_, index_key_);

        if (index_data) {
            boost::json::error_code error_code;
            auto parsed = boost::json::parse(*index_data, error_code);
            if (error_code) {
                // TODO: Log?
            } else {
                return boost::json::value_to<persistence::ContextIndex>(
                    std::move(parsed));
            }
        }
    }
    return persistence::ContextIndex();
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
