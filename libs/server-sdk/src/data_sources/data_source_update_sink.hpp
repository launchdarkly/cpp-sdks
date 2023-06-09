#pragma once

#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>

#include <launchdarkly/client_side/data_source_status.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_result.hpp>

namespace launchdarkly::client_side {

/**
 * An item descriptor is an abstraction that allows for Flag data to be
 * handled using the same type in both a put or a patch.
 */
struct ItemDescriptor {
    /**
     * The version number of this data, provided by the SDK.
     */
    uint64_t version;

    /**
     * The data item, or nullopt if this is a deleted item placeholder.
     */
    std::optional<EvaluationResult> flag;

    explicit ItemDescriptor(uint64_t version);

    explicit ItemDescriptor(EvaluationResult flag);

    ItemDescriptor(ItemDescriptor const& item) = default;
    ItemDescriptor(ItemDescriptor&& item) = default;
    ItemDescriptor& operator=(ItemDescriptor const&) = default;
    ItemDescriptor& operator=(ItemDescriptor&&) = default;
    ~ItemDescriptor() = default;

    friend std::ostream& operator<<(std::ostream& out,
                                    ItemDescriptor const& descriptor);
};

/**
 * Interface for handling updates from LaunchDarkly.
 */
class IDataSourceUpdateSink {
   public:
    virtual void Init(Context const& context,
                      std::unordered_map<std::string, ItemDescriptor> data) = 0;
    virtual void Upsert(Context const& context,
                        std::string key,
                        ItemDescriptor item) = 0;

    IDataSourceUpdateSink(IDataSourceUpdateSink const& item) = delete;
    IDataSourceUpdateSink(IDataSourceUpdateSink&& item) = delete;
    IDataSourceUpdateSink& operator=(IDataSourceUpdateSink const&) = delete;
    IDataSourceUpdateSink& operator=(IDataSourceUpdateSink&&) = delete;
    virtual ~IDataSourceUpdateSink() = default;

   protected:
    IDataSourceUpdateSink() = default;
};

bool operator==(ItemDescriptor const& lhs, ItemDescriptor const& rhs);

}  // namespace launchdarkly::client_side
