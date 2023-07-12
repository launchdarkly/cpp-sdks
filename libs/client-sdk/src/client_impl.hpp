#pragma once

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <thread>
#include <tuple>

#include "tl/expected.hpp"

#include <launchdarkly/client_side/client.hpp>
#include <launchdarkly/client_side/data_source_status.hpp>
#include <launchdarkly/client_side/flag_notifier.hpp>
#include <launchdarkly/config/client.hpp>
#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_detail.hpp>
#include <launchdarkly/data_sources/data_source.hpp>
#include <launchdarkly/error.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/value.hpp>

#include "data_sources/data_source_status_manager.hpp"
#include "event_processor.hpp"
#include "flag_manager/flag_manager.hpp"

namespace launchdarkly::client_side {
class ClientImpl : public IClient {
   public:
    ClientImpl(Config config, Context context, std::string const& version);

    ClientImpl(ClientImpl&&) = delete;
    ClientImpl(ClientImpl const&) = delete;
    ClientImpl& operator=(ClientImpl) = delete;
    ClientImpl& operator=(ClientImpl&& other) = delete;

    bool Initialized() const override;

    using FlagKey = std::string;
    [[nodiscard]] std::unordered_map<FlagKey, Value> AllFlags() const override;

    void Track(std::string event_name,
               Value data,
               double metric_value) override;

    void Track(std::string event_name, Value data) override;

    void Track(std::string event_name) override;

    void FlushAsync() override;

    std::future<bool> IdentifyAsync(Context context) override;

    bool BoolVariation(FlagKey const& key, bool default_value) override;

    EvaluationDetail<bool> BoolVariationDetail(FlagKey const& key,
                                               bool default_value) override;

    std::string StringVariation(FlagKey const& key,
                                std::string default_value) override;

    EvaluationDetail<std::string> StringVariationDetail(
        FlagKey const& key,
        std::string default_value) override;

    double DoubleVariation(FlagKey const& key, double default_value) override;

    EvaluationDetail<double> DoubleVariationDetail(
        FlagKey const& key,
        double default_value) override;

    int IntVariation(FlagKey const& key, int default_value) override;

    EvaluationDetail<int> IntVariationDetail(FlagKey const& key,
                                             int default_value) override;

    Value JsonVariation(FlagKey const& key, Value default_value) override;

    EvaluationDetail<Value> JsonVariationDetail(FlagKey const& key,
                                                Value default_value) override;

    data_sources::IDataSourceStatusProvider& DataSourceStatus() override;

    flag_manager::IFlagNotifier& FlagNotifier() override;

    ~ClientImpl();

    std::future<bool> StartAsync() override;

   private:
    template <typename T>
    [[nodiscard]] EvaluationDetail<T> VariationInternal(FlagKey const& key,
                                                        Value default_value,
                                                        bool check_type,
                                                        bool detailed);
    void TrackInternal(std::string event_name,
                       std::optional<Value> data,
                       std::optional<double> metric_value);

    template <typename F>
    auto ReadContextSynchronized(F fn) const
        -> std::invoke_result_t<F, Context const&> {
        std::shared_lock lock(context_mutex_);
        return fn(context_);
    }

    void UpdateContextSynchronized(Context context);

    void RestartDataSource();

    std::future<bool> StartAsyncInternal(
        std::function<bool(data_sources::DataSourceStatus::DataSourceState)>
            predicate);

    Config config_;
    Logger logger_;

    launchdarkly::config::shared::built::HttpProperties http_properties_;

    boost::asio::io_context ioc_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work_;

    Context context_;
    mutable std::shared_mutex context_mutex_;

    flag_manager::FlagManager flag_manager_;
    std::function<std::shared_ptr<::launchdarkly::data_sources::IDataSource>()>
        data_source_factory_;

    std::shared_ptr<::launchdarkly::data_sources::IDataSource> data_source_;

    std::unique_ptr<IEventProcessor> event_processor_;

    mutable std::mutex init_mutex_;
    std::condition_variable init_waiter_;

    data_sources::DataSourceStatusManager status_manager_;

    std::thread run_thread_;

    bool eval_reasons_available_;
};
}  // namespace launchdarkly::client_side
