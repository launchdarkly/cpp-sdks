#pragma once

#include <boost/asio/io_context.hpp>

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <thread>
#include <tl/expected.hpp>
#include <tuple>
#include "config/client.hpp"
#include "context.hpp"
#include "data/evaluation_detail.hpp"
#include "error.hpp"
#include "launchdarkly/client_side/data_source.hpp"
#include "launchdarkly/client_side/data_sources/detail/data_source_status_manager.hpp"
#include "launchdarkly/client_side/event_processor.hpp"
#include "launchdarkly/client_side/flag_manager/detail/flag_manager.hpp"
#include "launchdarkly/client_side/flag_manager/detail/flag_notifier.hpp"
#include "launchdarkly/client_side/flag_manager/detail/flag_updater.hpp"
#include "logger.hpp"
#include "value.hpp"

namespace launchdarkly::client_side {
class Client {
   public:
    Client(Config config, Context context);

    Client(Client&&) = delete;
    Client(Client const&) = delete;
    Client& operator=(Client) = delete;
    Client& operator=(Client&& other) = delete;

    bool Initialized() const;

    using FlagKey = std::string;
    [[nodiscard]] std::unordered_map<FlagKey, Value> AllFlags() const;

    void Track(std::string event_name, Value data, double metric_value);

    void Track(std::string event_name, Value data);

    void Track(std::string event_name);

    void AsyncFlush();

    /**
     * Re-initializes the client connection using a new context. Only one
     * call to AsyncIdentify may be active at once.
     * @param context
     */
    void AsyncIdentify(Context context, std::function<void()> completion);

    bool BoolVariation(FlagKey const& key, bool default_value);

    EvaluationDetail<bool> BoolVariationDetail(FlagKey const& key,
                                               bool default_value);

    std::string StringVariation(FlagKey const& key, std::string default_value);

    EvaluationDetail<std::string> StringVariationDetail(
        FlagKey const& key,
        std::string default_value);

    double DoubleVariation(FlagKey const& key, double default_value);

    EvaluationDetail<double> DoubleVariationDetail(FlagKey const& key,
                                                   double default_value);

    int IntVariation(FlagKey const& key, int default_value);

    EvaluationDetail<int> IntVariationDetail(FlagKey const& key,
                                             int default_value);

    Value JsonVariation(FlagKey const& key, Value default_value);

    EvaluationDetail<Value> JsonVariationDetail(FlagKey const& key,
                                                Value default_value);

    data_sources::IDataSourceStatusProvider& DataSourceStatus();

    flag_manager::detail::IFlagNotifier& FlagNotifier();

    void WaitForReadySync(std::chrono::milliseconds timeout);

    ~Client();

   private:
    template <typename T>
    [[nodiscard]] EvaluationDetail<T> VariationInternal(FlagKey const& key,
                                                        Value default_value,
                                                        bool check_type,
                                                        bool detailed);
    void TrackInternal(std::string event_name,
                       std::optional<Value> data,
                       std::optional<double> metric_value);

    template <typename T>
    T ReadContext(std::function<T(Context const&)> fn) const {
        std::shared_lock lock(context_mutex_);
        return fn(context_);
    }

    void WriteContext(Context context);

    void OnDataSourceShutdown(Context context,
                              std::function<void()> user_completion);

    Config config_;

    Logger logger_;
    boost::asio::io_context ioc_;

    Context context_;
    mutable std::shared_mutex context_mutex_;

    std::function<std::unique_ptr<IDataSource>()> data_source_factory_;

    std::unique_ptr<IDataSource> data_source_;

    std::unique_ptr<IEventProcessor> event_processor_;

    flag_manager::detail::FlagManager flag_manager_;
    flag_manager::detail::FlagUpdater flag_updater_;

    bool initialized_;
    mutable std::mutex init_mutex_;
    std::condition_variable init_waiter_;

    data_sources::detail::DataSourceStatusManager status_manager_;

    std::thread thread_;

    std::thread run_thread_;

    bool eval_reasons_available_;
};

}  // namespace launchdarkly::client_side
