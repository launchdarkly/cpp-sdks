#pragma once

#include "../../data_interfaces/destination/itransactional_destination.hpp"
#include "../../data_interfaces/item_change.hpp"
#include "../../data_interfaces/store/istore.hpp"

#include <launchdarkly/data_model/change_set.hpp>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_components {

class MemoryStore final : public data_interfaces::IStore,
                          public data_interfaces::ITransactionalDestination {
   public:
    [[nodiscard]] std::shared_ptr<data_model::FlagDescriptor> GetFlag(
        std::string const& key) const override;

    [[nodiscard]] std::shared_ptr<data_model::SegmentDescriptor> GetSegment(
        std::string const& key) const override;

    [[nodiscard]] std::
        unordered_map<std::string, std::shared_ptr<data_model::FlagDescriptor>>
        AllFlags() const override;

    [[nodiscard]] std::unordered_map<
        std::string,
        std::shared_ptr<data_model::SegmentDescriptor>>
    AllSegments() const override;

    [[nodiscard]] bool Initialized() const override;

    [[nodiscard]] std::string const& Identity() const override;

    void Init(data_model::SDKDataSet dataSet) override;

    void Upsert(std::string const& key,
                data_model::FlagDescriptor flag) override;

    void Upsert(std::string const& key,
                data_model::SegmentDescriptor segment) override;

    bool RemoveFlag(std::string const& key);

    bool RemoveSegment(std::string const& key);

    void Apply(data_model::ChangeSet<data_interfaces::ChangeSetData> changeSet)
        override;

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

}  // namespace launchdarkly::server_side::data_components
