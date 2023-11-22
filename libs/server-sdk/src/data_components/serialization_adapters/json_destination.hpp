#pragma once

#include "../../data_components/kinds/kinds.hpp"
#include "../../data_interfaces/destination/idestination.hpp"
#include "../../data_interfaces/destination/iserialized_destination.hpp"

namespace launchdarkly::server_side::data_components {

class JsonDestination final : public data_interfaces::IDestination {
   public:
    explicit JsonDestination(
        data_interfaces::ISerializedDestination& destination);

    virtual void Init(data_model::SDKDataSet data_set) override;

    void Upsert(std::string const& key,
                data_model::FlagDescriptor flag) override;

    void Upsert(std::string const& key,
                data_model::SegmentDescriptor segment) override;

    [[nodiscard]] std::string const& Identity() const override;

   private:
    data_interfaces::ISerializedDestination& dest_;
    struct Kinds {
        static FlagKind const Flag;
        static SegmentKind const Segment;
    };
};

}  // namespace launchdarkly::server_side::data_components
