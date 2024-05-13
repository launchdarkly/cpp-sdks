#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>

#include <chrono>
#include <thread>

#include <launchdarkly/config/client.hpp>
#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/events/asio_event_processor.hpp>
#include <launchdarkly/events/detail/parse_date_header.hpp>
#include <launchdarkly/logging/console_backend.hpp>

using namespace launchdarkly::events;
using namespace launchdarkly::events::detail;
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

    WorkerPool pool(ioc.get_executor(), 1, std::chrono::seconds(1),
                    VerifyMode::kVerifyPeer, logger);

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

    WorkerPool pool(ioc.get_executor(), 0, std::chrono::seconds(1),
                    VerifyMode::kVerifyPeer, logger);

    RequestWorker* worker = pool.Get(boost::asio::use_future).get();
    ASSERT_FALSE(worker);

    work.reset();
    ioc_thread.join();
}

class EventProcessorTests : public ::testing::Test {
   public:
    EventProcessorTests() : locale("en_US.utf-8") {}
    std::locale locale;
};

// This test is a temporary test that exists only to ensure the event processor
// compiles; it should be replaced by more robust tests (and contract tests.)
TEST_F(EventProcessorTests, ProcessorCompiles) {
    using namespace launchdarkly;

    Logger logger{
        std::make_shared<logging::ConsoleBackend>(LogLevel::kDebug, "test")};
    boost::asio::io_context ioc;

    auto config_builder = client_side::ConfigBuilder("sdk-123");
    config_builder.Events().Capacity(10).FlushInterval(std::chrono::seconds(1));
    auto config = config_builder.Build();

    ASSERT_TRUE(config);

    events::AsioEventProcessor<client_side::SDK> processor(
        ioc.get_executor(), config->ServiceEndpoints(), config->Events(),
        config->HttpProperties(), logger);
    std::thread ioc_thread([&]() { ioc.run(); });

    auto context = launchdarkly::ContextBuilder().Kind("org", "ld").Build();
    ASSERT_TRUE(context.Valid());

    auto identify_event = events::IdentifyEventParams{
        std::chrono::system_clock::now(),
        context,
    };

    for (std::size_t i = 0; i < 10; i++) {
        processor.SendAsync(identify_event);
    }

    processor.ShutdownAsync();
    ioc_thread.join();
}

TEST_F(EventProcessorTests, ParseValidDateHeader) {
    using namespace launchdarkly;

    using Clock = std::chrono::system_clock;
    auto date = events::detail::ParseDateHeader<Clock>(
        "Wed, 21 Oct 2015 07:28:00 GMT", locale);

    ASSERT_TRUE(date);

    ASSERT_EQ(date->time_since_epoch(),
              std::chrono::microseconds(1445412480000000));
}

TEST_F(EventProcessorTests, ParseInvalidDateHeader) {
    using namespace launchdarkly;

    auto not_a_date =
        events::detail::ParseDateHeader<std::chrono::system_clock>(
            "this is definitely not a date", locale);

    ASSERT_FALSE(not_a_date);

    auto not_gmt = events::detail::ParseDateHeader<std::chrono::system_clock>(
        "Wed, 21 Oct 2015 07:28:00 PST", locale);

    ASSERT_FALSE(not_gmt);

    auto missing_year =
        events::detail::ParseDateHeader<std::chrono::system_clock>(
            "Wed, 21 Oct 07:28:00 GMT", locale);

    ASSERT_FALSE(missing_year);
}
