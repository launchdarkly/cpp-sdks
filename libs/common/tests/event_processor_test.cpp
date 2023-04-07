#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <thread>
#include "config/client.hpp"
#include "console_backend.hpp"
#include "context_builder.hpp"
#include "events/detail/asio_event_processor.hpp"
#include "null_logger.hpp"
using namespace launchdarkly;
class EventProcessorTests : public ::testing::Test {};

TEST_F(EventProcessorTests, thing) {
    Logger logger{std::make_unique<ConsoleBackend>(LogLevel::kDebug, "test")};
    boost::asio::io_context io;

    auto config = client::EventsBuilder()
                      .capacity(10)
                      .flush_interval(std::chrono::seconds(1))
                      .build();

    auto endpoints = client::HostsBuilder().build();

    events::detail::AsioEventProcessor ep(io.get_executor(), *config,
                                          *endpoints, logger);

    std::thread t([&]() {
        io.run();
        std::cout << "exiting";
    });
    std::this_thread::sleep_for(std::chrono::seconds(2));

    auto c = launchdarkly::ContextBuilder().kind("org", "ld").build();
    auto ev = events::CustomEvent{events::BaseEvent{.context = c}};

    ep.async_send(ev);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    ep.async_send(ev);

    ep.async_close();
    t.join();
}
