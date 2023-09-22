
#include "../descriptors.hpp"
#include "../interfaces/data_destination.hpp"
#include "../interfaces/serialized_data_destination.hpp"

namespace launchdarkly::server_side::data_retrieval::adapters {

class JsonDestination : public IDataDestination {
   public:
    JsonDestination(ISerializedDataDestination& destination);

    void Init(data_model::SDKDataSet data_set) override;
    void Upsert(std::string& key, FlagDescriptor flag) override;
    void Upsert(std::string& key, SegmentDescriptor segment) override;
    std::string Identity() const override;

   private:
    ISerializedDataDestination& dest_;
};

}  // namespace launchdarkly::server_side::data_retrieval::adapters
