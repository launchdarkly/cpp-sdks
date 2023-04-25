#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <thread>
#include "config/client.hpp"
#include "console_backend.hpp"
#include "context_builder.hpp"
#include "events/client_events.hpp"
#include "events/detail/asio_event_processor.hpp"

using namespace launchdarkly::events::detail;

static std::chrono::system_clock::time_point TimeZero() {
    return std::chrono::system_clock::time_point{};
}

static std::chrono::system_clock::time_point Time1000() {
    return std::chrono::system_clock::from_time_t(1);
}

// This test is a temporary test that exists only to ensure the event processor
// compiles; it should be replaced by more robust tests (and contract tests.)
TEST(EventProcessorTests, ProcessorCompiles) {
    using namespace launchdarkly;

    Logger logger{std::make_unique<ConsoleBackend>(LogLevel::kDebug, "test")};
    boost::asio::io_context ioc;

    auto config = client_side::EventsBuilder()
                      .Capacity(10)
                      .FlushInterval(std::chrono::seconds(1))
                      .Build();

    auto endpoints = client_side::EndpointsBuilder().Build();

    auto http_props = client_side::HttpPropertiesBuilder().Build();

    events::detail::AsioEventProcessor processor(
        ioc.get_executor(), *config, *endpoints, http_props, "password", logger);
    std::thread ioc_thread([&]() { ioc.run(); });

    auto context = launchdarkly::ContextBuilder().kind("org", "ld").build();
    ASSERT_TRUE(context.valid());

    auto identify_event = events::client::IdentifyEventParams{
        std::chrono::system_clock::now(),
        context,
    };

    for (std::size_t i = 0; i < 10; i++) {
        processor.AsyncSend(identify_event);
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));
    processor.AsyncClose();
    ioc_thread.join();
}
