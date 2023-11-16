#pragma once

#include "../store/istore.hpp"

namespace launchdarkly::server_side::data_interfaces {

/**
 * \brief ISystem obtains data used for flag evaluations and makes it available
 * to other components.
 */
class ISystem : public IStore {
   public:
    /**
     * \return Identity of the system. Used in logs.
     */
    [[nodiscard]] virtual std::string const& Identity() const = 0;

    /**
     * \brief Initializes the system. This method will be called before any of
     * the IStore methods are called.
     */
    virtual void Initialize() = 0;

    virtual ~ISystem() = default;
    ISystem(ISystem const& item) = delete;
    ISystem(ISystem&& item) = delete;
    ISystem& operator=(ISystem const&) = delete;
    ISystem& operator=(ISystem&&) = delete;

   protected:
    ISystem() = default;
};

}  // namespace launchdarkly::server_side::data_interfaces
