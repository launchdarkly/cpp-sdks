#include "flag_store.hpp"

namespace launchdarkly::server_side::flag_manager {

// TODO(cwaldren): This is copy-pasted from the client-side SDK. It may be able
// to be shared.

// Shared pointers are used to item descriptors so that they may have a lifetime
// greater than their duration in the store. If, for instance, a flag has been
// accessed, and which it is being used init is called, then we want the
// flag being processed to be valid.

void FlagStore::Init(
    std::unordered_map<std::string, FlagItemDescriptor> const& flags,
    std::unordered_map<std::string, SegmentItemDescriptor> const& segments) {
    UpdateData(flags, segments);
}

void FlagStore::UpdateData(
    std::unordered_map<std::string, FlagItemDescriptor> const& flags,
    std::unordered_map<std::string, SegmentItemDescriptor> const& segments) {
    std::lock_guard lock{data_mutex_};
    this->flags_.clear();
    for (auto item : flags) {
        this->flags_.emplace(item.first, std::make_shared<FlagItemDescriptor>(
                                             std::move(item.second)));
    }
    this->segments_.clear();
    for (auto item : segments) {
        this->segments_.emplace(
            item.first,
            std::make_shared<SegmentItemDescriptor>(std::move(item.second)));
    }
}

void FlagStore::Upsert(std::string const& key, FlagItemDescriptor item) {
    std::lock_guard lock{data_mutex_};

    flags_[key] = std::make_shared<FlagItemDescriptor>(std::move(item));
}

void FlagStore::Upsert(std::string const& key, SegmentItemDescriptor item) {
    std::lock_guard lock{data_mutex_};

    segments_[key] = std::make_shared<SegmentItemDescriptor>(std::move(item));
}

std::shared_ptr<FlagItemDescriptor> FlagStore::GetFlag(
    std::string const& flag_key) const {
    std::lock_guard lock{data_mutex_};

    auto found = flags_.find(flag_key);
    if (found != flags_.end()) {
        return found->second;
    }
    return nullptr;
}

std::shared_ptr<SegmentItemDescriptor> FlagStore::GetSegment(
    std::string const& flag_key) const {
    std::lock_guard lock{data_mutex_};

    auto found = segments_.find(flag_key);
    if (found != segments_.end()) {
        return found->second;
    }
    return nullptr;
}

std::unordered_map<std::string, std::shared_ptr<FlagItemDescriptor>>
FlagStore::GetAll() const {
    std::lock_guard lock{data_mutex_};

    // Returns a copy of the map. (The descriptors are pointers and not shared).
    return flags_;
}

}  // namespace launchdarkly::server_side::flag_manager
