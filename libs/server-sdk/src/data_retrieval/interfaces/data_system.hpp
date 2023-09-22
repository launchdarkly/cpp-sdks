#pragma once

#include <launchdarkly/data_model/descriptors.hpp>
#include <launchdarkly/data_model/sdk_data_set.hpp>

#include "data_store/data_store.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_retrieval {

class IDataSystem : public IDataStore {
   public:
    [[nodiscard]] virtual std::string const& Identity() const = 0;
    virtual void Initialize() = 0;

    virtual ~IDataSystem() = default;
    IDataSystem(IDataSystem const& item) = delete;
    IDataSystem(IDataSystem&& item) = delete;
    IDataSystem& operator=(IDataSystem const&) = delete;
    IDataSystem& operator=(IDataSystem&&) = delete;

   protected:
    IDataSystem() = default;
};

}  // namespace launchdarkly::server_side::data_retrieval
