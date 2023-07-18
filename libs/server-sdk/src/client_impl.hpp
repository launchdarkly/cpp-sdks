#pragma once

#include <launchdarkly/config/client.hpp>
#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_detail.hpp>
#include <launchdarkly/data_sources/data_source.hpp>
#include <launchdarkly/error.hpp>
#include <launchdarkly/events/event_processor_interface.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/server_side/client.hpp>
#include <launchdarkly/value.hpp>

#include "data_sources/data_source_status_manager.hpp"
#include "data_sources/data_source_update_sink.hpp"

#include "data_store/memory_store.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>

#include <tl/expected.hpp>

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <thread>
#include <tuple>

namespace launchdarkly::server_side {

class ClientImpl : public IClient {
   public:
    ClientImpl(Config config, std::string const& version);

    ClientImpl(ClientImpl&&) = delete;
    ClientImpl(ClientImpl const&) = delete;
    ClientImpl& operator=(ClientImpl) = delete;
    ClientImpl& operator=(ClientImpl&& other) = delete;

    bool Initialized() const override;

    using FlagKey = std::string;
    [[nodiscard]] std::unordered_map<FlagKey, Value> AllFlagsState()
        const override;

    void Track(Context const& ctx,
               std::string event_name,
               Value data,
               double metric_value) override;

    void Track(Context const& ctx, std::string event_name, Value data) override;

    void Track(Context const& ctx, std::string event_name) override;

    void FlushAsync() override;

    void Identify(Context context) override;

    bool BoolVariation(Context const& ctx,
                       FlagKey const& key,
                       bool default_value) override;

    EvaluationDetail<bool> BoolVariationDetail(Context const& ctx,
                                               FlagKey const& key,
                                               bool default_value) override;

    std::string StringVariation(Context const& ctx,
                                FlagKey const& key,
                                std::string default_value) override;

    EvaluationDetail<std::string> StringVariationDetail(
        Context const& ctx,
        FlagKey const& key,
        std::string default_value) override;

    double DoubleVariation(Context const& ctx,
                           FlagKey const& key,
                           double default_value) override;

    EvaluationDetail<double> DoubleVariationDetail(
        Context const& ctx,
        FlagKey const& key,
        double default_value) override;

    int IntVariation(Context const& ctx,
                     FlagKey const& key,
                     int default_value) override;

    EvaluationDetail<int> IntVariationDetail(Context const& ctx,
                                             FlagKey const& key,
                                             int default_value) override;

    Value JsonVariation(Context const& ctx,
                        FlagKey const& key,
                        Value default_value) override;

    EvaluationDetail<Value> JsonVariationDetail(Context const& ctx,
                                                FlagKey const& key,
                                                Value default_value) override;

    ~ClientImpl();

    std::future<bool> StartAsync() override;

   private:
    template <typename T>
    [[nodiscard]] EvaluationDetail<T> VariationInternal(Context const& ctx,
                                                        FlagKey const& key,
                                                        Value default_value,
                                                        bool check_type,
                                                        bool detailed);
    void TrackInternal(Context const& ctx,
                       std::string event_name,
                       std::optional<Value> data,
                       std::optional<double> metric_value);

    std::future<bool> StartAsyncInternal(
        std::function<bool(data_sources::DataSourceStatus::DataSourceState)>
            predicate);

    Config config_;
    Logger logger_;

    launchdarkly::config::shared::built::HttpProperties http_properties_;

    boost::asio::io_context ioc_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work_;

    data_store::MemoryStore memory_store_;

    std::shared_ptr<::launchdarkly::data_sources::IDataSource> data_source_;

    std::unique_ptr<events::IEventProcessor> event_processor_;

    mutable std::mutex init_mutex_;
    std::condition_variable init_waiter_;

    data_sources::DataSourceStatusManager status_manager_;

    std::thread run_thread_;
};
}  // namespace launchdarkly::server_side