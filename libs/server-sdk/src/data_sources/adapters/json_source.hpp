
#include "../descriptors.hpp"
#include "../interfaces/data_source_lite.hpp"
#include "../interfaces/serialized_data_source.hpp"

namespace launchdarkly::server_side::data_sources::adapters {

class JsonSource : public IDataSourceLite {
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

}  // namespace launchdarkly::server_side::data_sources::adapters
