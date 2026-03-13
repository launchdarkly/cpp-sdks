#pragma once

#include "../store/istore.hpp"

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
     * @brief Returns true if the data system is capable of serving
     * flag evaluations even when Initialized() returns false.
     *
     * This is the case for Lazy Load (daemon mode), where data can be
     * fetched on-demand from the persistent store regardless of whether
     * the $inited key has been set. In contrast, Background Sync
     * cannot serve evaluations until initial data is received.
     *
     * When this returns true, the evaluation path should log a warning
     * (rather than returning CLIENT_NOT_READY) if Initialized() is false.
     */
    [[nodiscard]] virtual bool CanEvaluateWhenNotInitialized() const {
        return false;
    }

    virtual ~IDataSystem() override = default;
    IDataSystem(IDataSystem const& item) = delete;
    IDataSystem(IDataSystem&& item) = delete;
    IDataSystem& operator=(IDataSystem const&) = delete;
    IDataSystem& operator=(IDataSystem&&) = delete;

   protected:
    IDataSystem() = default;
};

}  // namespace launchdarkly::server_side::data_interfaces
