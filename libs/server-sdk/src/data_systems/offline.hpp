#pragma once

#include "../data_components/status_notifications/data_source_status_manager.hpp"
#include "../data_interfaces/system/idata_system.hpp"

namespace launchdarkly::server_side::data_systems {

class OfflineSystem final : public data_interfaces::IDataSystem {
   public:
    OfflineSystem(data_components::DataSourceStatusManager& status_manager);
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

   private:
    data_components::DataSourceStatusManager& status_manager_;
};

}  // namespace launchdarkly::server_side::data_systems
