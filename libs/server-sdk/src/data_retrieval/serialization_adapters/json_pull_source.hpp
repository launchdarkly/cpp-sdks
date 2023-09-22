
#include "../descriptors.hpp"
#include "../interfaces/data_pull_source.hpp"
#include "../interfaces/serialzied_data_pull_source.hpp"

namespace launchdarkly::server_side::data_system::adapters {

class JsonSource : public IPullSource {
   public:
    FlagDescriptor GetFlag(std::string& key) const override;
    SegmentDescriptor GetSegment(std::string& key) const override;
    std::unordered_map<std::string, FlagDescriptor> AllFlags() const override;
    std::unordered_map<std::string, SegmentDescriptor> AllSegments()
        const override;
    std::string Identity() const override;

   public:
   private:
    ISerializedDataSource& source_;
};

}  // namespace launchdarkly::server_side::data_system::adapters
