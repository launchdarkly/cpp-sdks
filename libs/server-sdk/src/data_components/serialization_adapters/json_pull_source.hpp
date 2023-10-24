#include "../../data_interfaces/destination/idestination.hpp"
#include "../../data_interfaces/destination/iserialized_destination.hpp"

#include "../../data_interfaces/source/ipull_source.hpp"
#include "../../data_interfaces/source/iserialized_pull_source.hpp"

namespace launchdarkly::server_side::data_components {

class JsonSource : public data_interfaces::IPullSource {
   public:
    data_model::FlagDescriptor GetFlag(std::string& key) const override;
    data_model::SegmentDescriptor GetSegment(std::string& key) const override;
    std::unordered_map<std::string, data_model::FlagDescriptor> AllFlags()
        const override;
    std::unordered_map<std::string, data_model::SegmentDescriptor> AllSegments()
        const override;
    std::string Identity() const override;

   public:
   private:
    data_interfaces::ISerializedDataPullSource& source_;
};

}  // namespace launchdarkly::server_side::data_components
