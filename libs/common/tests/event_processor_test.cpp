#include <gtest/gtest.h>
#include "events/null_event_processor.hpp"

using namespace launchdarkly::events;
class EventProcessorTests : public ::testing::Test {};

TEST_F(EventProcessorTests, thing) {
    NullEventProcessor p;
    p.async_close();
}
