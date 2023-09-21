#pragma once
#include "../descriptors.hpp"

#include <launchdarkly/data_model/sdk_data_set.hpp>

#include <tl/expected.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_sources {

class IBootstrapper {
   public:
    using Error = std::string;
    virtual bool IsAuthoritative() const = 0;
    virtual tl::expected<data_model::SDKDataSet, Error> FetchAll(
        std::chrono::milliseconds timeout_hint) = 0;
    virtual std::string const& Identity() const = 0;
    virtual ~IBootstrapper() = default;
    IBootstrapper(IBootstrapper const& item) = delete;
    IBootstrapper(IBootstrapper&& item) = delete;
    IBootstrapper& operator=(IBootstrapper const&) = delete;
    IBootstrapper& operator=(IBootstrapper&&) = delete;

   protected:
    IBootstrapper() = default;
};

}  // namespace launchdarkly::server_side::data_sources
