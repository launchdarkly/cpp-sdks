

#include "memory_store.hpp"

namespace launchdarkly::server_side::data_retrieval {

std::shared_ptr<FlagDescriptor> MemoryStore::GetFlag(
    std::string const& key) const {
    std::lock_guard lock{data_mutex_};
    auto found = flags_.find(key);
    if (found != flags_.end()) {
        return found->second;
    }
    return nullptr;
}

std::shared_ptr<SegmentDescriptor> MemoryStore::GetSegment(
    std::string const& key) const {
    std::lock_guard lock{data_mutex_};
    auto found = segments_.find(key);
    if (found != segments_.end()) {
        return found->second;
    }
    return nullptr;
}

std::unordered_map<std::string, std::shared_ptr<FlagDescriptor>>
MemoryStore::AllFlags() const {
    std::lock_guard lock{data_mutex_};
    return {flags_};
}

std::unordered_map<std::string, std::shared_ptr<SegmentDescriptor>>
MemoryStore::AllSegments() const {
    std::lock_guard lock{data_mutex_};
    return {segments_};
}

bool MemoryStore::Initialized() const {
    std::lock_guard lock{data_mutex_};
    return initialized_;
}

std::string const& MemoryStore::Description() const {
    return description_;
}

void MemoryStore::Init(launchdarkly::data_model::SDKDataSet dataSet) {
    std::lock_guard lock{data_mutex_};
    initialized_ = true;
    flags_.clear();
    segments_.clear();
    for (auto flag : dataSet.flags) {
        flags_.emplace(flag.first, std::make_shared<FlagDescriptor>(
                                       std::move(flag.second)));
    }
    for (auto segment : dataSet.segments) {
        segments_.emplace(segment.first, std::make_shared<SegmentDescriptor>(
                                             std::move(segment.second)));
    }
}

void MemoryStore::Upsert(std::string const& key,
                         data_sources::FlagDescriptor flag) {
    std::lock_guard lock{data_mutex_};
    flags_[key] = std::make_shared<FlagDescriptor>(std::move(flag));
}

void MemoryStore::Upsert(std::string const& key,
                         data_sources::SegmentDescriptor segment) {
    std::lock_guard lock{data_mutex_};
    segments_[key] = std::make_shared<SegmentDescriptor>(std::move(segment));
}

}  // namespace launchdarkly::server_side::data_retrieval
