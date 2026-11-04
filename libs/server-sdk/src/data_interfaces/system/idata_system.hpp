#pragma once

#include "../store/istore.hpp"

#include <optional>
#include <string>

namespace launchdarkly::server_side::data_interfaces {

/**
 * @brief IDataSystem obtains data used for flag evaluations and makes it
 * available to other components.
 */
class IDataSystem : public IStore {
   public:
    /**
     * @return Identity of the system. Used in logs.
     */
    [[nodiscard]] virtual std::string const& Identity() const = 0;

    /**
     * @brief Initializes the system. This method will be called before any of
     * the IStore methods are called.
     */
    virtual void Initialize() = 0;

    /**
     * @return The environment ID reported by LaunchDarkly alongside the data,
     * if the system has received one.
     */
    [[nodiscard]] virtual std::optional<std::string> EnvironmentId() const = 0;

    virtual ~IDataSystem() override = default;
    IDataSystem(IDataSystem const& item) = delete;
    IDataSystem(IDataSystem&& item) = delete;
    IDataSystem& operator=(IDataSystem const&) = delete;
    IDataSystem& operator=(IDataSystem&&) = delete;

   protected:
    IDataSystem() = default;
};

}  // namespace launchdarkly::server_side::data_interfaces
