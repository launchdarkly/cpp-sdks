#pragma once

#include "../../data_sources/data_source_update_sink.hpp"
#include "../data_store.hpp"
#include "../memory_store.hpp"
#include "expiration_tracker.hpp"

#include <launchdarkly/server_side/integrations/persistent_store_core.hpp>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_store::persistent {

class PersistentStore : public IDataStore,
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

    PersistentStore() = default;
    ~PersistentStore() override = default;

    PersistentStore(PersistentStore const& item) = delete;
    PersistentStore(PersistentStore&& item) = delete;
    PersistentStore& operator=(PersistentStore const&) = delete;
    PersistentStore& operator=(PersistentStore&&) = delete;

   private:
    MemoryStore memory_store_;
    std::shared_ptr<integrations::IPersistentStoreCore> persistent_store_core_;
    ExpirationTracker ttl_tracker_;
};

}  // namespace launchdarkly::server_side::data_store::persistent