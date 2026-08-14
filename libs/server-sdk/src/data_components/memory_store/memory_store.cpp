#include "memory_store.hpp"

#include <launchdarkly/detail/unreachable.hpp>

namespace launchdarkly::server_side::data_components {

std::shared_ptr<data_model::FlagDescriptor> MemoryStore::GetFlag(
    std::string const& key) const {
    std::lock_guard lock{data_mutex_};
    auto found = flags_.find(key);
    if (found != flags_.end()) {
        return found->second;
    }
    return nullptr;
}

std::shared_ptr<data_model::SegmentDescriptor> MemoryStore::GetSegment(
    std::string const& key) const {
    std::lock_guard lock{data_mutex_};
    auto found = segments_.find(key);
    if (found != segments_.end()) {
        return found->second;
    }
    return nullptr;
}

std::unordered_map<std::string, std::shared_ptr<data_model::FlagDescriptor>>
MemoryStore::AllFlags() const {
    std::lock_guard lock{data_mutex_};
    return {flags_};
}

std::unordered_map<std::string, std::shared_ptr<data_model::SegmentDescriptor>>
MemoryStore::AllSegments() const {
    std::lock_guard lock{data_mutex_};
    return {segments_};
}

bool MemoryStore::Initialized() const {
    std::lock_guard lock{data_mutex_};
    return initialized_;
}

std::string const& MemoryStore::Identity() const {
    return description_;
}

void MemoryStore::Init(data_model::SDKDataSet dataSet) {
    std::lock_guard lock{data_mutex_};
    initialized_ = true;
    flags_.clear();
    segments_.clear();
    for (auto flag : dataSet.flags) {
        flags_.emplace(flag.first, std::make_shared<data_model::FlagDescriptor>(
                                       std::move(flag.second)));
    }
    for (auto segment : dataSet.segments) {
        segments_.emplace(segment.first,
                          std::make_shared<data_model::SegmentDescriptor>(
                              std::move(segment.second)));
    }
}

void MemoryStore::Upsert(std::string const& key,
                         data_model::FlagDescriptor flag) {
    std::lock_guard lock{data_mutex_};
    flags_[key] = std::make_shared<data_model::FlagDescriptor>(std::move(flag));
}

void MemoryStore::Upsert(std::string const& key,
                         data_model::SegmentDescriptor segment) {
    std::lock_guard lock{data_mutex_};
    segments_[key] =
        std::make_shared<data_model::SegmentDescriptor>(std::move(segment));
}

bool MemoryStore::RemoveFlag(std::string const& key) {
    std::lock_guard lock{data_mutex_};
    return flags_.erase(key) == 1;
}

bool MemoryStore::RemoveSegment(std::string const& key) {
    std::lock_guard lock{data_mutex_};
    return segments_.erase(key) == 1;
}

void MemoryStore::Apply(
    data_model::ChangeSet<data_interfaces::ChangeSetData> changeSet) {
    std::lock_guard lock{data_mutex_};

    switch (changeSet.type) {
        case data_model::ChangeSetType::kNone:
            return;
        case data_model::ChangeSetType::kPartial:
            break;
        case data_model::ChangeSetType::kFull:
            initialized_ = true;
            flags_.clear();
            segments_.clear();
            break;
        default:
            detail::unreachable();
    }

    for (auto& change : changeSet.data) {
        if (std::holds_alternative<data_model::FlagDescriptor>(change.object)) {
            flags_[change.key] = std::make_shared<data_model::FlagDescriptor>(
                std::move(std::get<data_model::FlagDescriptor>(change.object)));
        } else if (std::holds_alternative<data_model::SegmentDescriptor>(
                       change.object)) {
            segments_[change.key] =
                std::make_shared<data_model::SegmentDescriptor>(std::move(
                    std::get<data_model::SegmentDescriptor>(change.object)));
        }
    }
}

}  // namespace launchdarkly::server_side::data_components
