#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>

#include <chrono>
#include <thread>

#include <launchdarkly/config/client.hpp>
#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/events/asio_event_processor.hpp>
#include <launchdarkly/events/client_events.hpp>
#include <launchdarkly/events/parse_date_header.hpp>
#include <launchdarkly/events/worker_pool.hpp>
#include <launchdarkly/logging/console_backend.hpp>

using namespace launchdarkly::events;
using namespace launchdarkly::network;

static std::chrono::system_clock::time_point TimeZero() {
    return std::chrono::system_clock::time_point{};
}

static std::chrono::system_clock::time_point Time1000() {
    return std::chrono::system_clock::from_time_t(1);
}

TEST(WorkerPool, PoolReturnsAvailableWorker) {
    using namespace launchdarkly;
    Logger logger{
        std::make_shared<logging::ConsoleBackend>(LogLevel::kDebug, "test")};
    boost::asio::io_context ioc;

    auto work = boost::asio::make_work_guard(ioc);
    std::thread ioc_thread([&]() { ioc.run(); });

    WorkerPool pool(ioc.get_executor(), 1, std::chrono::seconds(1), logger);

    RequestWorker* worker = pool.Get(boost::asio::use_future).get();
    ASSERT_TRUE(worker);

    work.reset();
    ioc_thread.join();
}

TEST(WorkerPool, PoolReturnsNullptrWhenNoWorkerAvaialable) {
    using namespace launchdarkly;
    Logger logger{
        std::make_shared<logging::ConsoleBackend>(LogLevel::kDebug, "test")};
    boost::asio::io_context ioc;

    auto work = boost::asio::make_work_guard(ioc);
    std::thread ioc_thread([&]() { ioc.run(); });

    WorkerPool pool(ioc.get_executor(), 0, std::chrono::seconds(1), logger);

    RequestWorker* worker = pool.Get(boost::asio::use_future).get();
    ASSERT_FALSE(worker);

    work.reset();
    ioc_thread.join();
}

// This test is a temporary test that exists only to ensure the event processor
// compiles; it should be replaced by more robust tests (and contract tests.)
TEST(EventProcessorTests, ProcessorCompiles) {
    using namespace launchdarkly;

    Logger logger{
        std::make_shared<logging::ConsoleBackend>(LogLevel::kDebug, "test")};
    boost::asio::io_context ioc;

    auto config_builder = client_side::ConfigBuilder("sdk-123");
    config_builder.Events().Capacity(10).FlushInterval(std::chrono::seconds(1));
    auto config = config_builder.Build();

    ASSERT_TRUE(config);

    events::AsioEventProcessor processor(ioc.get_executor(), *config, logger);
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

TEST(EventProcessorTests, ParseValidDateHeader) {
    using namespace launchdarkly;

    using Clock = std::chrono::system_clock;
    auto date = events::ParseDateHeader<Clock>("Wed, 21 Oct 2015 07:28:00 GMT");

    ASSERT_TRUE(date);

    ASSERT_EQ(date->time_since_epoch(),
              std::chrono::microseconds(1445412480000000));
}

TEST(EventProcessorTests, ParseInvalidDateHeader) {
    using namespace launchdarkly;

    auto not_a_date = events::ParseDateHeader<std::chrono::system_clock>(
        "this is definitely not a date");

    ASSERT_FALSE(not_a_date);

    auto not_gmt = events::ParseDateHeader<std::chrono::system_clock>(
        "Wed, 21 Oct 2015 07:28:00 PST");

    ASSERT_FALSE(not_gmt);

    auto missing_year = events::ParseDateHeader<std::chrono::system_clock>(
        "Wed, 21 Oct 07:28:00 GMT");

    ASSERT_FALSE(missing_year);
}
