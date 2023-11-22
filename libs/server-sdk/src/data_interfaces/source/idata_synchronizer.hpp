#pragma once

#include <launchdarkly/data_model/sdk_data_set.hpp>

#include <functional>
#include <optional>
#include <string>

#include "../destination/idestination.hpp"

namespace launchdarkly::server_side::data_interfaces {

/**
 * \brief IDataSynchronizer obtains data via a background synchronization
 * mechanism, updating a local cache whenever changes are made upstream.
 */
class IDataSynchronizer {
   public:
    /**
     * @brief Starts synchronizing data into the given IDestination.
     *
     *
     * The second parameter, boostrap_data, may be nullptr meaning no bootstrap
     * data is present in the SDK and a full synchronization must be initiated.
     *
     * If bootstrap_data is not nullptr, then it contains data obtained by the
     * SDK out-of-band from the source's mechanism. The pointer is valid only
     * for this call.
     *
     * The data may be used to optimize the synchronization process, e.g. by
     * obtaining a diff rather than a full dataset.
     * @param destination The destination to synchronize data into. Pointer is
     * invalid after the ShutdownAsync completion handler is called.
     * @param bootstrap_data Optional bootstrap data.
     * Pointer is valid only for this call.
     */
    virtual void StartAsync(IDestination* destination,
                            data_model::SDKDataSet const* bootstrap_data) = 0;

    /**
     * \brief Stops the synchronization mechanism. Stop will be called only once
     * after StartAsync. Stop should not block, but should invoke the completion
     * function once shutdown.
     * \param complete A callback to be invoked on completion.
     */
    virtual void ShutdownAsync(std::function<void()> complete) = 0;

    /**
     * \return Identity of the source. Used in logs.
     */
    [[nodiscard]] virtual std::string const& Identity() const = 0;

    virtual ~IDataSynchronizer() = default;
    IDataSynchronizer(IDataSynchronizer const& item) = delete;
    IDataSynchronizer(IDataSynchronizer&& item) = delete;
    IDataSynchronizer& operator=(IDataSynchronizer const&) = delete;
    IDataSynchronizer& operator=(IDataSynchronizer&&) = delete;

   protected:
    IDataSynchronizer() = default;
};

}  // namespace launchdarkly::server_side::data_interfaces
