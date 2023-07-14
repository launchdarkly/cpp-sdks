#pragma once

#include "../data_sources/data_source_update_sink.hpp"
#include "data_store.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_store {

class MemoryStore : public IDataStore,
                    public data_sources::IDataSourceUpdateSink {
   public:
    std::shared_ptr<FlagDescriptor> GetFlag(
        std::string const& key) const override;
    std::shared_ptr<SegmentDescriptor> GetSegment(
        std::string const& key) const override;

    std::unordered_map<std::string, std::shared_ptr<FlagDescriptor>> AllFlags()
        const override;
    std::unordered_map<std::string, std::shared_ptr<SegmentDescriptor>>
    AllSegments() const override;

    bool Initialized() const override;
    std::string const& Description() const override;

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
    static inline const std::string description_ = "memory";
    std::unordered_map<std::string, std::shared_ptr<FlagDescriptor>> flags_;
    std::unordered_map<std::string, std::shared_ptr<SegmentDescriptor>>
        segments_;
    bool initialized_ = false;
    mutable std::mutex data_mutex_;
};

}  // namespace launchdarkly::server_side::data_store
