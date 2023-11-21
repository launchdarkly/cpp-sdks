#pragma once

#include <launchdarkly/data_model/sdk_data_set.hpp>

#include <functional>
#include <optional>
#include <string>

namespace launchdarkly::server_side::data_interfaces {

/**
 * \brief IDataSynchronizer obtains data via a background synchronization
 * mechanism, updating a local cache whenever changes are made upstream.
 */
class IDataSynchronizer {
   public:
    /**
     * \brief Initialize the source, optionally with an initial data set. Init
     * will be called before Start.
     * \param initial_data Initial set of SDK data.
     */
    virtual void Init(std::optional<data_model::SDKDataSet> initial_data) = 0;

    /**
     * \brief Starts the synchronization mechanism. Start will be called only
     * once after Init; the source is responsible for maintaining a persistent
     * connection. Start should not block.
     */
    virtual void StartAsync() = 0;

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
