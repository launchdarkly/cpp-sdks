#include "../../data_interfaces/destination/idestination.hpp"
#include "../../data_interfaces/destination/iserialized_destination.hpp"

namespace launchdarkly::server_side::data_components {

class JsonDestination : public data_interfaces::IDestination {
   public:
    JsonDestination(data_interfaces::ISerializedDestination& destination);

    void Init(data_model::SDKDataSet data_set) override;
    void Upsert(std::string& key, data_model::FlagDescriptor flag) override;
    void Upsert(std::string& key,
                data_model::SegmentDescriptor segment) override;
    std::string Identity() const override;

   private:
    data_interfaces::ISerializedDestination& dest_;
};

}  // namespace launchdarkly::server_side::data_components
