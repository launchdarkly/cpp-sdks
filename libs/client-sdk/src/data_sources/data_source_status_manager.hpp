#pragma once

#include <functional>
#include <mutex>

#include <boost/signals2.hpp>

#include <launchdarkly/client_side/data_source_status.hpp>
#include <launchdarkly/connection.hpp>

namespace launchdarkly::client_side::data_sources {

/**
 * Class that manages updates to the data source status and implements an
 * interface to get the current status and listen to status changes.
 */
class DataSourceStatusManager : public IDataSourceStatusProvider {
   public:
    using DataSourceStatusHandler =
        std::function<void(DataSourceStatus status)>;

    /**
     * Set the state.
     *
     * @param state The new state.
     */
    void SetState(DataSourceStatus::DataSourceState state);

    /**
     * If an error and state change happen simultaneously, then they should
     * be updated simultaneously.
     *
     * @param state The new state.
     * @param code Status code for an http error.
     * @param message The message to associate with the error.
     */
    void SetState(DataSourceStatus::DataSourceState state,
                  DataSourceStatus::ErrorInfo::StatusCodeType code,
                  std::string message);

    /**
     * If an error and state change happen simultaneously, then they should
     * be updated simultaneously.
     *
     * @param state The new state.
     * @param kind The error kind.
     * @param message The message to associate with the error.
     */
    void SetState(DataSourceStatus::DataSourceState state,
                  DataSourceStatus::ErrorInfo::ErrorKind kind,
                  std::string message);

    /**
     * Set an error with the given kind and message.
     *
     * For ErrorInfo::ErrorKind::kErrorResponse use the
     * SetError(ErrorInfo::StatusCodeType) method.
     * @param kind The kind of the error.
     * @param message A message for the error.
     */
    void SetError(DataSourceStatus::ErrorInfo::ErrorKind kind,
                  std::string message);

    /**
     * Set an error based on the given status code.
     * @param code The status code of the error.
     */
    void SetError(DataSourceStatus::ErrorInfo::StatusCodeType code,
                  std::string message);
    // TODO: Handle error codes once the EventSource supports it.

    DataSourceStatus Status() override;

    std::unique_ptr<IConnection> OnDataSourceStatusChange(
        std::function<void(data_sources::DataSourceStatus status)> handler)
        override;

    DataSourceStatusManager();

   private:
    DataSourceStatus::DataSourceState state_;
    DataSourceStatus::DateTime state_since_;
    std::optional<DataSourceStatus::ErrorInfo> last_error_;

    boost::signals2::signal<void(data_sources::DataSourceStatus status)>
        data_source_status_signal_;
    mutable std::recursive_mutex status_mutex_;
    bool UpdateState(DataSourceStatus::DataSourceState const& requested_state);
};

}  // namespace launchdarkly::client_side::data_sources
