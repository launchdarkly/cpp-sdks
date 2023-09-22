#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include "../data_destination_interface.hpp"

namespace launchdarkly::server_side::data_system {

class MemoryStore : public data_sources::IDataDestination {
   public:
    std::shared_ptr<FlagDescriptor> GetFlag(std::string const& key) const;
    std::shared_ptr<SegmentDescriptor> GetSegment(std::string const& key) const;

    std::unordered_map<std::string, std::shared_ptr<FlagDescriptor>> AllFlags()
        const;
    std::unordered_map<std::string, std::shared_ptr<SegmentDescriptor>>
    AllSegments() const;

    bool Initialized() const;
    std::string const& Description() const;

    void Init(launchdarkly::data_model::SDKDataSet dataSet) override;
    void Upsert(std::string const& key, FlagDescriptor flag) override;
    void Upsert(std::string const& key, SegmentDescriptor segment) override;

    MemoryStore() = default;
    ~MemoryStore() override = default;

    MemoryStore(MemoryStore const& item) = delete;
    MemoryStore(MemoryStore&& item) = delete;
    MemoryStore& operator=(MemoryStore const&) = delete;
    MemoryStore& operator=(MemoryStore&&) = delete;

   private:
    static inline std::string const description_ = "memory";
    std::unordered_map<std::string, std::shared_ptr<FlagDescriptor>> flags_;
    std::unordered_map<std::string, std::shared_ptr<SegmentDescriptor>>
        segments_;
    bool initialized_ = false;
    mutable std::mutex data_mutex_;
};

}  // namespace launchdarkly::server_side::data_system
