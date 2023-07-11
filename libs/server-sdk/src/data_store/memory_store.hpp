#pragma once

#include "../data_source/data_source_update_sink.hpp"
#include "data_store.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_store {

class MemoryStore : public IDataStore,
                    public data_source::IDataSourceUpdateSink {
   public:
    std::shared_ptr<IDataStore::FlagDescriptor> GetFlag(
        std::string const& key) const override;
    std::shared_ptr<IDataStore::SegmentDescriptor> GetSegment(
        std::string const& key) const override;

    std::unordered_map<std::string, std::shared_ptr<IDataStore::FlagDescriptor>>
    AllFlags() const override;
    std::unordered_map<std::string,
                       std::shared_ptr<IDataStore::SegmentDescriptor>>
    AllSegments() const override;

    bool Initialized() const override;
    std::string const& Description() const override;

    void Init(launchdarkly::data_model::SDKDataSet dataSet) override;
    void Upsert(std::string key, IDataStore::FlagDescriptor flag) override;
    void Upsert(std::string key,
                IDataStore::SegmentDescriptor segment) override;

    ~MemoryStore() override = default;

   private:
    static inline std::string description_ = "memory";
    std::unordered_map<std::string, std::shared_ptr<IDataStore::FlagDescriptor>>
        flags_;
    std::unordered_map<std::string,
                       std::shared_ptr<IDataStore::SegmentDescriptor>>
        segments_;
    bool initialized_ = false;
    mutable std::mutex data_mutex_;
};

}  // namespace launchdarkly::server_side::data_store
