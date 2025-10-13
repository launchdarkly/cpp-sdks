#include <gtest/gtest.h>
#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/server_side/client.hpp>
#include <launchdarkly/server_side/config/config_builder.hpp>
#include <map>

using namespace launchdarkly;
using namespace launchdarkly::server_side;

class ClientTest : public ::testing::Test {
   protected:
    ClientTest()
        : client_(ConfigBuilder("sdk-123").Build().value()),
          context_(ContextBuilder().Kind("cat", "shadow").Build()) {}

    Client client_;
    Context const context_;
};

TEST_F(ClientTest, ClientConstructedWithMinimalConfigAndContextT) {
    char const* version = client_.Version();
    ASSERT_TRUE(version);
    ASSERT_STREQ(version, "3.9.1");  // {x-release-please-version}
}

TEST_F(ClientTest, BoolVariationDefaultPassesThrough) {
    std::string const flag = "extra-cat-food";
    std::vector<bool> values = {true, false};
    for (auto const& v : values) {
        ASSERT_EQ(client_.BoolVariation(context_, flag, v), v);
        ASSERT_EQ(*client_.BoolVariationDetail(context_, flag, v), v);
    }
}

TEST_F(ClientTest, StringVariationDefaultPassesThrough) {
    std::string const flag = "treat";
    std::vector<std::string> values = {"chicken", "fish", "cat-grass"};
    for (auto const& v : values) {
        ASSERT_EQ(client_.StringVariation(context_, flag, v), v);
        ASSERT_EQ(*client_.StringVariationDetail(context_, flag, v), v);
    }
}

TEST_F(ClientTest, IntVariationDefaultPassesThrough) {
    std::string const flag = "weight";
    std::vector<int> values = {0, 12, 13, 24, 1000};
    for (auto const& v : values) {
        ASSERT_EQ(client_.IntVariation(context_, flag, v), v);
        ASSERT_EQ(*client_.IntVariationDetail(context_, flag, v), v);
    }
}

TEST_F(ClientTest, DoubleVariationDefaultPassesThrough) {
    std::string const flag = "weight";
    std::vector<double> values = {0.0,  0.0001, 0.5,  1.234,
                                  12.0, 13.0,   24.0, 1000.0};
    for (auto const& v : values) {
        ASSERT_EQ(client_.DoubleVariation(context_, flag, v), v);
        ASSERT_EQ(*client_.DoubleVariationDetail(context_, flag, v), v);
    }
}

TEST_F(ClientTest, JsonVariationDefaultPassesThrough) {
    std::string const flag = "assorted-values";
    std::vector<Value> values = {
        Value({"running", "jumping"}), Value(3), Value(1.0), Value(true),
        Value(std::map<std::string, Value>{{"weight", 20}})};
    for (auto const& v : values) {
        ASSERT_EQ(client_.JsonVariation(context_, flag, v), v);
        ASSERT_EQ(*client_.JsonVariationDetail(context_, flag, v), v);
    }
}

TEST_F(ClientTest, AllFlagsStateNotValid) {
    // Since we don't have any ability to insert into the data store, assert
    // only that the state is not valid.
    auto flags = client_.AllFlagsState(
        context_, AllFlagsState::Options::IncludeReasons |
                      AllFlagsState::Options::ClientSideOnly);
    ASSERT_FALSE(flags.Valid());
}
