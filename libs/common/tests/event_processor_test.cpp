#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <thread>
#include "config/client.hpp"
#include "console_backend.hpp"
#include "context_builder.hpp"
#include "events/detail/asio_event_processor.hpp"

using namespace launchdarkly;
class EventProcessorTests : public ::testing::Test {};

// This test is a temporary test that exists only to ensure the event processor
// compiles; it should be replaced by more robust tests (and contract tests.)
TEST_F(EventProcessorTests, ProcessorCompiles) {
    Logger logger{std::make_unique<ConsoleBackend>(LogLevel::kDebug, "test")};
    boost::asio::io_context io;

    auto config = client::EventsBuilder()
                      .capacity(10)
                      .flush_interval(std::chrono::seconds(1))
                      .build();

    auto endpoints = client::HostsBuilder().build();

    events::detail::AsioEventProcessor ep(io.get_executor(), *config,
                                          *endpoints, "password", logger);
    std::thread t([&]() { io.run(); });

    auto c = launchdarkly::ContextBuilder().kind("org", "ld").build();
    ASSERT_TRUE(c.valid());

    auto ev = events::client::IdentifyEventParams{
        std::chrono::system_clock::now(),
        c,
    };

    for (std::size_t i = 0; i < 10; i++) {
        ep.AsyncSend(ev);
    }

    ep.AsyncClose();
    t.join();
}
