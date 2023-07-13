#pragma once

#include <functional>
#include <mutex>

#include <boost/signals2.hpp>

#include <launchdarkly/connection.hpp>
#include <launchdarkly/signals/boost_signal_connection.hpp>

namespace launchdarkly::internal::data_sources {

/**
 * Class that manages updates to the data source status and implements an
 * interface to get the current status and listen to status changes.
 */
template <typename TDataSourceStatus, typename TInterface>
class DataSourceStatusManagerBase: public TInterface {
   public:
    /**
     * Set the state.
     *
     * @param state The new state.
     */
    void SetState(typename TDataSourceStatus::DataSourceState state) {
        bool changed = UpdateState(state);
        if (changed) {
            data_source_status_signal_(std::move(Status()));
        }
    }

    /**
     * If an error and state change happen simultaneously, then they should
     * be updated simultaneously.
     *
     * @param state The new state.
     * @param code Status code for an http error.
     * @param message The message to associate with the error.
     */
    void SetState(typename TDataSourceStatus::DataSourceState state,
                  typename TDataSourceStatus::ErrorInfo::StatusCodeType code,
                  std::string message) {
        {
            std::lock_guard lock(status_mutex_);

            UpdateState(state);

            last_error_ = typename TDataSourceStatus::ErrorInfo(
                TDataSourceStatus::ErrorInfo::ErrorKind::kErrorResponse, code,
                message, std::chrono::system_clock::now());
        }

        data_source_status_signal_(std::move(Status()));
    }

    /**
     * If an error and state change happen simultaneously, then they should
     * be updated simultaneously.
     *
     * @param state The new state.
     * @param kind The error kind.
     * @param message The message to associate with the error.
     */
    void SetState(typename TDataSourceStatus::DataSourceState state,
                  typename TDataSourceStatus::ErrorInfo::ErrorKind kind,
                  std::string message) {
        {
            std::lock_guard lock(status_mutex_);

            UpdateState(state);

            last_error_ = typename TDataSourceStatus::ErrorInfo(
                kind, 0, std::move(message), std::chrono::system_clock::now());
        }

        data_source_status_signal_(Status());
    }

    /**
     * Set an error with the given kind and message.
     *
     * For ErrorInfo::ErrorKind::kErrorResponse use the
     * SetError(ErrorInfo::StatusCodeType) method.
     * @param kind The kind of the error.
     * @param message A message for the error.
     */
    void SetError(typename TDataSourceStatus::ErrorInfo::ErrorKind kind,
                  std::string message) {
        {
            std::lock_guard lock(status_mutex_);
            last_error_ = typename TDataSourceStatus::ErrorInfo(
                kind, 0, std::move(message), std::chrono::system_clock::now());
            state_since_ = std::chrono::system_clock::now();
        }

        data_source_status_signal_(Status());
    }

    /**
     * Set an error based on the given status code.
     * @param code The status code of the error.
     */
    void SetError(typename TDataSourceStatus::ErrorInfo::StatusCodeType code,
                  std::string message) {
        {
            std::lock_guard lock(status_mutex_);
            last_error_ = typename TDataSourceStatus::ErrorInfo(
                TDataSourceStatus::ErrorInfo::ErrorKind::kErrorResponse, code,
                message, std::chrono::system_clock::now());
            state_since_ = std::chrono::system_clock::now();
        }
        data_source_status_signal_(Status());
    }

    TDataSourceStatus Status() const override {
        return {state_, state_since_, last_error_};
    }

    std::unique_ptr<IConnection> OnDataSourceStatusChange(
        std::function<void(TDataSourceStatus status)> handler) override {
        std::lock_guard lock{status_mutex_};
        return std::make_unique<
            ::launchdarkly::internal::signals::SignalConnection>(
            data_source_status_signal_.connect(handler));
    }

    std::unique_ptr<IConnection> OnDataSourceStatusChangeEx(
        std::function<bool(TDataSourceStatus status)> handler) override {
        return std::make_unique<
            launchdarkly::internal::signals::SignalConnection>(
            data_source_status_signal_.connect_extended(
                [handler](boost::signals2::connection const& conn,
                          TDataSourceStatus status) {
                    if (handler(status)) {
                        conn.disconnect();
                    }
                }));
    }
    DataSourceStatusManagerBase()
        : state_(TDataSourceStatus::DataSourceState::kInitializing),
          state_since_(std::chrono::system_clock::now()) {}

    virtual ~DataSourceStatusManagerBase() = default;
    DataSourceStatusManagerBase(DataSourceStatusManagerBase const& item) = delete;
    DataSourceStatusManagerBase(DataSourceStatusManagerBase&& item) = delete;
    DataSourceStatusManagerBase& operator=(DataSourceStatusManagerBase const&) =
        delete;
    DataSourceStatusManagerBase& operator=(DataSourceStatusManagerBase&&) = delete;

   private:
    typename TDataSourceStatus::DataSourceState state_;
    typename TDataSourceStatus::DateTime state_since_;
    std::optional<typename TDataSourceStatus::ErrorInfo> last_error_;

    boost::signals2::signal<void(TDataSourceStatus status)>
        data_source_status_signal_;
    mutable std::recursive_mutex status_mutex_;

    bool UpdateState(
        typename TDataSourceStatus::DataSourceState const& requested_state) {
        std::lock_guard lock(status_mutex_);

        // Interrupted and initializing are common to server and client.
        // If logic specific to client or server states was needed, then
        // the implementation would need re-organized to allow overriding the
        // method.

        // If initializing, then interruptions remain initializing.
        auto new_state =
            (requested_state ==
                 TDataSourceStatus::DataSourceState::kInterrupted &&
             state_ == TDataSourceStatus::DataSourceState::kInitializing)
                ? TDataSourceStatus::DataSourceState::
                      kInitializing  // see comment on
                                     // IDataSourceUpdateSink.UpdateStatus
                : requested_state;
        auto changed = state_ != new_state;
        if (changed) {
            state_ = new_state;
            state_since_ = std::chrono::system_clock::now();
        }
        return changed;
    }
};

}  // namespace launchdarkly::internal::data_sources
