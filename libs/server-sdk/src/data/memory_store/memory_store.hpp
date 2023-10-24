#pragma once

#include "../interfaces/data_dest/data_destination.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data {

class MemoryStore : public IDataDestination {
   public:
    std::shared_ptr<data_model::FlagDescriptor> GetFlag(
        std::string const& key) const;

    std::shared_ptr<data_model::SegmentDescriptor> GetSegment(
        std::string const& key) const;

    std::unordered_map<std::string, std::shared_ptr<data_model::FlagDescriptor>>
    AllFlags() const;
    std::unordered_map<std::string,
                       std::shared_ptr<data_model::SegmentDescriptor>>
    AllSegments() const;

    bool Initialized() const;
    std::string const& Description() const;

    void Init(launchdarkly::data_model::SDKDataSet dataSet) override;
    void Upsert(std::string const& key,
                data_model::FlagDescriptor flag) override;
    void Upsert(std::string const& key,
                data_model::SegmentDescriptor segment) override;

    MemoryStore() = default;
    ~MemoryStore() override = default;

    MemoryStore(MemoryStore const& item) = delete;
    MemoryStore(MemoryStore&& item) = delete;
    MemoryStore& operator=(MemoryStore const&) = delete;
    MemoryStore& operator=(MemoryStore&&) = delete;

   private:
    static inline std::string const description_ = "memory";
    std::unordered_map<std::string, std::shared_ptr<data_model::FlagDescriptor>>
        flags_;
    std::unordered_map<std::string,
                       std::shared_ptr<data_model::SegmentDescriptor>>
        segments_;
    bool initialized_ = false;
    mutable std::mutex data_mutex_;
};

}  // namespace launchdarkly::server_side::data
