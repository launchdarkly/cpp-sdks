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

ApplyResult MemoryStore::Apply(data_model::FDv2ChangeSet changeSet) {
    std::lock_guard lock{data_mutex_};
    ApplyResult result;

    switch (changeSet.type) {
        case data_model::FDv2ChangeSet::Type::kNone:
            return result;
        case data_model::FDv2ChangeSet::Type::kPartial:
            break;
        case data_model::FDv2ChangeSet::Type::kFull:
            // When there's a full change, any current keys are considered
            // changed, regardless of whether they are in the new set.
            for (auto const& [key, _] : flags_) {
                result.flags.insert(key);
            }
            for (auto const& [key, _] : segments_) {
                result.segments.insert(key);
            }
            initialized_ = true;
            flags_.clear();
            segments_.clear();
            break;
        default:
            detail::unreachable();
    }

    for (auto& change : changeSet.changes) {
        if (std::holds_alternative<data_model::FlagDescriptor>(change.object)) {
            auto& flag_descriptor =
                std::get<data_model::FlagDescriptor>(change.object);

            auto existing_flag = flags_.find(change.key);
            if (existing_flag != flags_.end() &&
                existing_flag->second->version >= flag_descriptor.version) {
                continue;
            }

            flags_[change.key] = std::make_shared<data_model::FlagDescriptor>(
                std::move(flag_descriptor));
            result.flags.insert(change.key);
        } else if (std::holds_alternative<data_model::SegmentDescriptor>(
                       change.object)) {
            auto& segment_descriptor =
                std::get<data_model::SegmentDescriptor>(change.object);

            auto existing_segment = segments_.find(change.key);
            if (existing_segment != segments_.end() &&
                existing_segment->second->version >=
                    segment_descriptor.version) {
                continue;
            }

            segments_[change.key] =
                std::make_shared<data_model::SegmentDescriptor>(
                    std::move(segment_descriptor));
            result.segments.insert(change.key);
        }
    }

    return result;
}

}  // namespace launchdarkly::server_side::data_components
