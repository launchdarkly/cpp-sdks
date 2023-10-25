#pragma once

#include "../store/istore.hpp"

#include <launchdarkly/data_model/descriptors.hpp>
#include <launchdarkly/data_model/sdk_data_set.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_interfaces {

class ISystem : public IStore {
   public:
    [[nodiscard]] virtual std::string const& Identity() const = 0;
    virtual void Initialize() = 0;

    [[nodiscard]] virtual IStore& Store() = 0;
    [[nodiscard]] virtual IStore const& Store() const = 0;

    virtual ~ISystem() = default;
    ISystem(ISystem const& item) = delete;
    ISystem(ISystem&& item) = delete;
    ISystem& operator=(ISystem const&) = delete;
    ISystem& operator=(ISystem&&) = delete;

   protected:
    ISystem() = default;
};

}  // namespace launchdarkly::server_side::data_interfaces
