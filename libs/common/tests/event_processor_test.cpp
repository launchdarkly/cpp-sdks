#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <launchdarkly/config/client.hpp>
#include <launchdarkly/console_backend.hpp>
#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/events/client_events.hpp>
#include <launchdarkly/events/detail/asio_event_processor.hpp>
#include <launchdarkly/events/detail/worker_pool.hpp>
#include <thread>

using namespace launchdarkly::events::detail;
using namespace launchdarkly::network::detail;

static std::chrono::system_clock::time_point TimeZero() {
    return std::chrono::system_clock::time_point{};
}

static std::chrono::system_clock::time_point Time1000() {
    return std::chrono::system_clock::from_time_t(1);
}

TEST(WorkerPool, PoolReturnsAvailableWorker) {
    using namespace launchdarkly;
    Logger logger{std::make_unique<ConsoleBackend>(LogLevel::kDebug, "test")};
    boost::asio::io_context ioc;

    auto work = boost::asio::make_work_guard(ioc);
    std::thread t([&]() { ioc.run(); });

    WorkerPool pool(ioc.get_executor(), 1, std::chrono::seconds(1), logger);

    RequestWorker* worker = pool.Get(boost::asio::use_future).get();
    ASSERT_TRUE(worker);

    work.reset();
    t.join();
}

TEST(WorkerPool, PoolReturnsNullptrWhenNoWorkerAvaialable) {
    using namespace launchdarkly;
    Logger logger{std::make_unique<ConsoleBackend>(LogLevel::kDebug, "test")};
    boost::asio::io_context ioc;

    auto work = boost::asio::make_work_guard(ioc);
    std::thread t([&]() { ioc.run(); });

    WorkerPool pool(ioc.get_executor(), 0, std::chrono::seconds(1), logger);

    RequestWorker* worker = pool.Get(boost::asio::use_future).get();
    ASSERT_FALSE(worker);

    work.reset();
    t.join();
}

// This test is a temporary test that exists only to ensure the event processor
// compiles; it should be replaced by more robust tests (and contract tests.)
TEST(EventProcessorTests, ProcessorCompiles) {
    using namespace launchdarkly;

    Logger logger{std::make_unique<ConsoleBackend>(LogLevel::kDebug, "test")};
    boost::asio::io_context ioc;

    auto config =
        client_side::ConfigBuilder("sdk-123")
            .Events(client_side::EventsBuilder().Capacity(10).FlushInterval(
                std::chrono::seconds(1)))
            .Build();

    ASSERT_TRUE(config);

    events::detail::AsioEventProcessor processor(ioc.get_executor(), *config,
                                                 logger);
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

    processor.AsyncClose();
    ioc_thread.join();
}
