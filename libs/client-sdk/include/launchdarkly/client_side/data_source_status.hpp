#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

#include <launchdarkly/connection.hpp>
#include <launchdarkly/data_sources/data_source_status_base.hpp>

namespace launchdarkly::client_side::data_sources {

/**
 * Enumeration of possible data source states.
 */
enum class ClientDataSourceState {
    /**
     * The initial state of the data source when the SDK is being
     * initialized.
     *
     * If it encounters an error that requires it to retry initialization,
     * the state will remain at kInitializing until it either succeeds and
     * becomes kValid, or permanently fails and becomes kShutdown.
     */
    kInitializing = 0,

    /**
     * Indicates that the data source is currently operational and has not
     * had any problems since the last time it received data.
     *
     * In streaming mode, this means that there is currently an open stream
     * connection and that at least one initial message has been received on
     * the stream. In polling mode, it means that the last poll request
     * succeeded.
     */
    kValid = 1,

    /**
     * Indicates that the data source encountered an error that it will
     * attempt to recover from.
     *
     * In streaming mode, this means that the stream connection failed, or
     * had to be dropped due to some other error, and will be retried after
     * a backoff delay. In polling mode, it means that the last poll request
     * failed, and a new poll request will be made after the configured
     * polling interval.
     */
    kInterrupted = 2,

    /**
     * Indicates that the application has told the SDK to stay offline.
     */
    kSetOffline = 3,

    /**
     * Indicates that the data source has been permanently shut down.
     *
     * This could be because it encountered an unrecoverable error (for
     * instance, the LaunchDarkly service rejected the SDK key; an invalid
     * SDK key will never become valid), or because the SDK client was
     * explicitly shut down.
     */
    kShutdown = 4,

    // BackgroundDisabled,
    // TODO: A plugin of sorts would likely be required for some
    // functionality like this. kNetworkUnavailable,
};

using DataSourceStatus =
    common::data_sources::DataSourceStatusBase<ClientDataSourceState>;

/**
 * Interface for accessing and listening to the data source status.
 */
class IDataSourceStatusProvider {
   public:
    /**
     * The current status of the data source. Suitable for broadcast to
     * data source status listeners.
     */
    [[nodiscard]] virtual DataSourceStatus Status() const = 0;

    /**
     * Listen to changes to the data source status.
     *
     * @param handler Function which will be called with the new status.
     * @return A IConnection which can be used to stop listening to the status.
     */
    virtual std::unique_ptr<IConnection> OnDataSourceStatusChange(
        std::function<void(data_sources::DataSourceStatus status)> handler) = 0;

    /**
     * Listen to changes to the data source status, with ability for listener
     * to unregister itself.
     *
     * @param handler Function which will be called with the new status. Return
     * true to unregister.
     * @return A IConnection which can be used to stop listening to the status.
     */
    virtual std::unique_ptr<IConnection> OnDataSourceStatusChangeEx(
        std::function<bool(data_sources::DataSourceStatus status)> handler) = 0;

    virtual ~IDataSourceStatusProvider() = default;
    IDataSourceStatusProvider(IDataSourceStatusProvider const& item) = delete;
    IDataSourceStatusProvider(IDataSourceStatusProvider&& item) = delete;
    IDataSourceStatusProvider& operator=(IDataSourceStatusProvider const&) =
        delete;
    IDataSourceStatusProvider& operator=(IDataSourceStatusProvider&&) = delete;

   protected:
    IDataSourceStatusProvider() = default;
};

std::ostream& operator<<(std::ostream& out,
                         DataSourceStatus::DataSourceState const& state);

std::ostream& operator<<(std::ostream& out, DataSourceStatus const& status);

}  // namespace launchdarkly::client_side::data_sources
