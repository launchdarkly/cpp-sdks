#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <thread>
#include "events/detail/asio_event_processor.hpp"

#include "null_logger.hpp"
using namespace launchdarkly::events;
class EventProcessorTests : public ::testing::Test {};

TEST_F(EventProcessorTests, thing) {
    auto logger = NullLogger();
    boost::asio::io_context io;
    AsioEventProcessor e(io.get_executor(),
                         launchdarkly::config::detail::Events{}, logger);
    std::thread t([&]() { io.run(); });
    std::this_thread::sleep_for(std::chrono::seconds(5));
    e.sync_close();
    t.join();
}
