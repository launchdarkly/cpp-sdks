#pragma once

#include "../destination/idestination.hpp"

#include <launchdarkly/data_model/descriptors.hpp>
#include <launchdarkly/data_model/sdk_data_set.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_interfaces {

class IPushSource {
   public:
    virtual void Init(std::optional<data_model::SDKDataSet> initial_data) = 0;
    virtual void Start() = 0;
    virtual void ShutdownAsync(std::function<void()>) = 0;

    [[nodiscard]] virtual std::string const& Identity() const = 0;

    virtual ~IPushSource() = default;
    IPushSource(IPushSource const& item) = delete;
    IPushSource(IPushSource&& item) = delete;
    IPushSource& operator=(IPushSource const&) = delete;
    IPushSource& operator=(IPushSource&&) = delete;

   protected:
    IPushSource() = default;
};

}  // namespace launchdarkly::server_side::data_interfaces
