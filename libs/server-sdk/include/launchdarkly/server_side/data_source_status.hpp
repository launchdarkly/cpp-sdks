#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <ostream>
#include <utility>

#include <launchdarkly/connection.hpp>
#include <launchdarkly/data_sources/data_source_status_base.hpp>

namespace launchdarkly::server_side {

/**
 * Enumeration of possible data source states.
 */
enum class DataSourceState {
    /**
     * The initial state of the data source when the SDK is being
     * initialized (when not offline, otherwise it will be kOffline.)
     *
     * During this time, the SDK serves default values for all flags.
     *
     * If it encounters an error that requires it to retry initialization,
     * the state will remain at kInitializing until it either succeeds and
     * becomes kInitialized, or permanently fails and becomes kOffline.
     */
    kInitializing = 0,

    /**
     * Indicates that the SDK has received initial data, and flag evaluations
     * will be based on that data.
     *
     * In this state, flag data may not be consistent with LaunchDarkly - for example,
     * if the initial data came from a local database.
     */
    kInitialized = 1,

    /**
     * Indicates that the SDK is reconciling the latest remote flag data
     * with the existing initial data. During this time, flag evaluations
     * will continue to be based on the initial data.
     */
    kReconciling = 2,

    /**
     * Indicates that the SDK has finished reconciliation and flag data is
     * now synchronized with LaunchDarkly. During this time, flag evaluations
     * are based on the latest flag data available.
     *
     * If an error occurs, the state will become kInterrupted.
     *
     */
    kTracking = 3,

    /**
     * Indicates that the data source encountered an error that it will
     * attempt to recover from.
     *
     * During this time, flag evaluations will continue to be based on the
     * latest data before the interruption.
     *
     * If the error is resolved, the state will become kTracking.
     */
    kInterrupted = 4,

    /**
     * Indicates that the data source has been permanently shut down.
     *
     * This could be because it encountered an unrecoverable error (for
     * instance, the LaunchDarkly service rejected the SDK key; an invalid
     * SDK key will never become valid), because the SDK client was
     * explicitly closed, or because the SDK's Data System was disabled (either
     * by explicit Data System configuration, or by the top-level Offline config.)
     */
    kOffline = 3,
};

using DataSourceStatus =
    common::data_sources::DataSourceStatusBase<DataSourceState>;

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
        std::function<void(DataSourceStatus status)> handler) = 0;

    /**
     * Listen to changes to the data source status, with ability for listener
     * to unregister itself.
     *
     * @param handler Function which will be called with the new status. Return
     * true to unregister.
     * @return A IConnection which can be used to stop listening to the status.
     */
    virtual std::unique_ptr<IConnection> OnDataSourceStatusChangeEx(
        std::function<bool(DataSourceStatus status)> handler) = 0;

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

}  // namespace launchdarkly::server_side
