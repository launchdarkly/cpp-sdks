#pragma once

#include "../data_interfaces/system/idata_system.hpp"

namespace launchdarkly::server_side::data_systems {

class OfflineSystem final : public data_interfaces::IDataSystem {
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
    void Initialize() override;
};

}  // namespace launchdarkly::server_side::data_systems
