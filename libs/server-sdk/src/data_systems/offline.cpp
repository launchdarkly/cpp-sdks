#include "offline.hpp"

namespace launchdarkly::server_side::data_systems {

std::shared_ptr<data_model::FlagDescriptor> OfflineSystem::GetFlag(
    std::string const& key) const {
    return nullptr;
}

std::shared_ptr<data_model::SegmentDescriptor> OfflineSystem::GetSegment(
    std::string const& key) const {
    return nullptr;
}

std::unordered_map<std::string, std::shared_ptr<data_model::FlagDescriptor>>
OfflineSystem::AllFlags() const {
    return std::unordered_map<std::string,
                              std::shared_ptr<data_model::FlagDescriptor>>();
}

std::unordered_map<std::string, std::shared_ptr<data_model::SegmentDescriptor>>
OfflineSystem::AllSegments() const {
    return std::unordered_map<std::string,
                              std::shared_ptr<data_model::SegmentDescriptor>>();
}

bool OfflineSystem::Initialized() const {
    return true;
}

std::string const& OfflineSystem::Identity() const {
    static std::string const ident = "offline";
    return ident;
}

void Initialize() {}

}  // namespace launchdarkly::server_side::data_systems
