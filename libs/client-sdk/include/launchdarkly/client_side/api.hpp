#pragma once

#include <boost/asio/io_context.hpp>

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <optional>
#include <thread>

#include <tl/expected.hpp>
#include "config/client.hpp"
#include "context.hpp"
#include "error.hpp"
#include "events/event_processor.hpp"
#include "launchdarkly/client_side/data_source.hpp"
#include "launchdarkly/client_side/data_sources/detail/data_source_status_manager.hpp"
#include "launchdarkly/client_side/flag_manager/detail/flag_manager.hpp"
#include "launchdarkly/client_side/flag_manager/detail/flag_updater.hpp"
#include "logger.hpp"
#include "value.hpp"

namespace launchdarkly::client_side {
class Client {
   public:
    Client(client::Config config, Context context);

    using FlagKey = std::string;
    [[nodiscard]] std::unordered_map<FlagKey, Value> AllFlags() const;

    void Track(std::string event_name, Value data, double metric_value);

    void Track(std::string event_name, Value data);

    void Track(std::string event_name);

    void AsyncFlush();

    void AsyncIdentify(Context context);

    bool BoolVariation(FlagKey const& key, bool default_value);

    std::string StringVariation(FlagKey const& key, std::string default_value);

    double DoubleVariation(FlagKey const& key, double default_value);

    int IntVariation(FlagKey const& key, int default_value);

    Value JsonVariation(FlagKey const& key, Value default_value);

    data_sources::IDataSourceStatusProvider* DataSourceStatus();

    void WaitForReadySync(std::chrono::seconds timeout);

    ~Client();

   private:
    Value VariationInternal(FlagKey const& key, Value default_value);
    void TrackInternal(std::string event_name,
                       std::optional<Value> data,
                       std::optional<double> metric_value);

    bool initialized_;
    std::mutex init_mutex_;
    std::condition_variable init_waiter_;

    data_sources::detail::DataSourceStatusManager status_manager_;
    flag_manager::detail::FlagManager flag_manager_;
    flag_manager::detail::FlagUpdater flag_updater_;

    Logger logger_;
    std::thread thread_;
    boost::asio::io_context ioc_;
    Context context_;
    std::unique_ptr<events::IEventProcessor> event_processor_;
    std::unique_ptr<IDataSource> data_source_;
    std::thread run_thread_;
};

}  // namespace launchdarkly::client_side
