

#include "memory_store.hpp"

namespace launchdarkly::server_side::data_store {

std::shared_ptr<IDataStore::FlagDescriptor> MemoryStore::GetFlag(
    std::string key) const {
    return std::shared_ptr<FlagDescriptor>();
}

std::shared_ptr<IDataStore::SegmentDescriptor> MemoryStore::GetSegment(
    std::string key) const {
    return std::shared_ptr<SegmentDescriptor>();
}

std::unordered_map<std::string, std::shared_ptr<IDataStore::FlagDescriptor>>
MemoryStore::AllFlags() const {
    return std::unordered_map<std::string, std::shared_ptr<FlagDescriptor>>();
}

std::unordered_map<std::string, std::shared_ptr<IDataStore::SegmentDescriptor>>
MemoryStore::AllSegments() const {
    return std::unordered_map<std::string,
                              std::shared_ptr<IDataStore::SegmentDescriptor>>();
}

bool MemoryStore::Initialized() const {
    return false;
}

std::string const& MemoryStore::Description() const {
    return description_;
}

}  // namespace launchdarkly::server_side::data_store
