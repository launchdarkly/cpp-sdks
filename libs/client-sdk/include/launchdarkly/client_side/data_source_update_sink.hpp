#pragma once

#include <map>
#include <string>

#include "config/detail/service_endpoints.hpp"
#include "context.hpp"
#include "data/evaluation_result.hpp"

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

    friend std::ostream& operator<<(std::ostream& out,
                                    ItemDescriptor const& descriptor);
};

/**
 * Interface for handling updates from LaunchDarkly.
 */
class IDataSourceUpdateSink {
   public:
    virtual void init(std::map<std::string, ItemDescriptor> data) = 0;
    virtual void upsert(std::string key, ItemDescriptor) = 0;

    // We could add this if we want to support data source status.
    // virtual void status(<something>)

    virtual ~IDataSourceUpdateSink() {}
};

bool operator==(ItemDescriptor const& lhs, ItemDescriptor const& rhs);

}  // namespace launchdarkly::client_side
