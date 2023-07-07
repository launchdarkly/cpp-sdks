#pragma once

#include "data_store.hpp"

#include <memory>
#include <string>

namespace launchdarkly::server_side::data_store {

class MemoryStore : public IDataStore {
   public:
    std::shared_ptr<FlagDescriptor> GetFlag(std::string key) const override;
    std::shared_ptr<SegmentDescriptor> GetSegment(
        std::string key) const override;
    std::unordered_map<std::string, std::shared_ptr<FlagDescriptor>> AllFlags()
        const override;
    std::unordered_map<std::string, std::shared_ptr<SegmentDescriptor>>
    AllSegments() const override;
    bool Initialized() const override;
    std::string const& Description() const override;
    ~MemoryStore() override = default;

   private:
    static inline std::string description_ = "Memory";
};

}  // namespace launchdarkly::server_side::data_store
