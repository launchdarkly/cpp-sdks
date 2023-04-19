#pragma once

#include <functional>

#include "launchdarkly/client_side/data_sources/data_source_status.hpp"

namespace launchdarkly::client_side::data_sources::detail {

class DataSourceStateManager {
   public:
    using DataSourceStatusHandler =
        std::function<void(DataSourceStatus status)>;

    /**
     * Set the state.
     *
     * @param state The new state.
     */
    void SetState(DataSourceState state);

    /**
     * Set the current error.
     *
     * @param error The error to set.
     */
    void SetError(std::optional<ErrorInfo> error);

    /**
     * The current status of the data source. Suitable for broadcast to
     * data source status listeners.
     */
    DataSourceStatus Status();

    DataSourceStateManager(DataSourceStatusHandler handler);

   private:
    DataSourceState state_;
    DataSourceStatus::DateTime state_since_;
    std::optional<ErrorInfo> last_error_;
    DataSourceStatusHandler handler_;
};

}  // namespace launchdarkly::client_side::data_sources::detail
