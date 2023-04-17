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
    Client(std::unique_ptr<ILogBackend> log_backend,
           std::string sdk_key,
           Context context,
           config::detail::Events const& event_config,
           config::ServiceEndpoints const& endpoints_config);

    using FlagKey = std::string;
    std::unordered_map<FlagKey, Value> AllFlags() const;

    void Track(std::string event_name, Value data, double metric_value);
    void Track(std::string event_name, Value data);
    void Track(std::string event_name);

    void AsyncFlush();

    void AsyncIdentify();

    bool BoolVariation(FlagKey key, bool default_value);
    std::string StringVariation(FlagKey key, std::string default_value);
    double DoubleVariation(FlagKey key, double default_value);
    int IntVariation(FlagKey key, int default_value);
    Value JsonVariation(FlagKey key, Value default_value);

   private:
    Logger logger_;
    std::thread thread_;
    boost::asio::io_context ioc_;
    Context context_;
    std::unique_ptr<events::IEventProcessor> event_processor_;
};

static tl::expected<Client, Error> Create(client::Config config,
                                          Context context);

}  // namespace launchdarkly::client_side
