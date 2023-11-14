#include <gtest/gtest.h>

#include "data_systems/lazy_load/lazy_load_system.hpp"

using namespace launchdarkly::server_side::data_systems;

class LazyLoadTest : public ::testing::Test {};

TEST_F(LazyLoadTest, Thing) {
    LazyLoad system;
    ASSERT_FALSE(system.GetFlag("foo"));
}
