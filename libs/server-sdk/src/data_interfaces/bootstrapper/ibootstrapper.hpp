#pragma once

#include <launchdarkly/data_model/sdk_data_set.hpp>

#include <tl/expected.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_interfaces {

/**
 * Defines a component that can fetch a complete dataset for use in a
 * Data System. Bootstrapping takes place when the SDK starts, and is
 * responsible for provisioning the initial data that an SDK uses before it can
 * begin the on-going synchronization process.
 */
class IBootstrapper {
   public:
    class Error {
       public:
        enum class Kind {
            None,
            Timeout,
            Auth,
        };

        static Error Timeout(std::string detail) {
            return Error(Kind::Timeout, std::move(detail));
        }

        static Error Auth(std::string detail) {
            return Error(Kind::Auth, std::move(detail));
        }

        Error() : kind(Kind::None), detail(std::nullopt) {}

       private:
        Error(Kind kind, std::optional<std::string> detail)
            : kind(kind), detail(std::move(detail)) {}
        Kind kind;
        std::optional<std::string> detail;
    };

    /**
     * Fetch a complete dataset. This method must invokable multiple times.
     * @param timeout_hint amount of time to spend fetching data. If the time
     * limit is reached, return Error::Timeout.
     * @return A complete SDKDataSet on success, or an error indicating why it
     * couldn't be retrieved.
     */
    virtual tl::expected<data_model::SDKDataSet, Error> FetchAll(
        std::chrono::milliseconds timeout_hint) = 0;

    /**
     * @return A display-suitable name of the bootstrapper.
     */
    virtual std::string const& Identity() const = 0;

    virtual ~IBootstrapper() = default;
    IBootstrapper(IBootstrapper const& item) = delete;
    IBootstrapper(IBootstrapper&& item) = delete;
    IBootstrapper& operator=(IBootstrapper const&) = delete;
    IBootstrapper& operator=(IBootstrapper&&) = delete;

   protected:
    IBootstrapper() = default;
};

}  // namespace launchdarkly::server_side::data_interfaces
