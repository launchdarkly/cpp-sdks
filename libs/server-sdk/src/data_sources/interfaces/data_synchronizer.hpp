#pragma once
#include "../descriptors.hpp"

#include <launchdarkly/data_model/sdk_data_set.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_sources {

class ISynchronizer {
   public:
    virtual void Init(std::optional<data_model::SDKDataSet> initial_data,
                      IDataDestination& destination) = 0;
    virtual void Start() = 0;
    virtual void ShutdownAsync(std::function<void()>) = 0;
    virtual ~ISynchronizer() = default;
    ISynchronizer(ISynchronizer const& item) = delete;
    ISynchronizer(ISynchronizer&& item) = delete;
    ISynchronizer& operator=(ISynchronizer const&) = delete;
    ISynchronizer& operator=(ISynchronizer&&) = delete;

   protected:
    ISynchronizer() = default;
};

}  // namespace launchdarkly::server_side::data_sources
