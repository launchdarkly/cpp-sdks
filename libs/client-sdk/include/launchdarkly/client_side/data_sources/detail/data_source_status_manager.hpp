#pragma once

#include <functional>

#include <boost/signals2.hpp>

#include "launchdarkly/client_side/connection.hpp"
#include "launchdarkly/client_side/data_sources/data_source_status.hpp"

namespace launchdarkly::client_side::data_sources::detail {

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
     * Set an error with the given kind and message.
     *
     * For ErrorInfo::ErrorKind::kErrorResponse use the
     * SetError(ErrorInfo::StatusCodeType) method.
     * @param kind The kind of the error.
     * @param message A message for the error.
     */
    void SetError(DataSourceStatus::ErrorInfo::ErrorKind kind,
                  std::string message);
    // TODO: Handle interrupted and other error states when they are
    // propagated from the event source.

    /**
     * Set an error based on the given status code.
     * @param code The status code of the error.
     */
    void SetError(DataSourceStatus::ErrorInfo::StatusCodeType code);
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
    mutable std::mutex status_mutex_;
};

}  // namespace launchdarkly::client_side::data_sources::detail
