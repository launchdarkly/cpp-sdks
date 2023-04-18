#pragma once

#include <boost/asio/io_context.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <thread>
#include <tl/expected.hpp>
#include "config/client.hpp"
#include "context.hpp"
#include "error.hpp"
#include "events/event_processor.hpp"
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

   private:
    void TrackInternal(std::string event_name,
                       std::optional<Value> data,
                       std::optional<double> metric_value);

    Logger logger_;
    std::thread thread_;
    boost::asio::io_context ioc_;
    Context context_;
    std::unique_ptr<events::IEventProcessor> event_processor_;
};

}  // namespace launchdarkly::client_side
