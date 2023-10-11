#include <gtest/gtest.h>
#include <launchdarkly/client_side/client.hpp>
#include <launchdarkly/context_builder.hpp>
#include <map>

using namespace launchdarkly;
using namespace launchdarkly::client_side;

TEST(ClientTest, ClientConstructedWithMinimalConfigAndContext) {
    tl::expected<Config, Error> config = ConfigBuilder("sdk-123").Build();
    ASSERT_TRUE(config);

    Context context = ContextBuilder().Kind("cat", "shadow").Build();

    Client client(std::move(*config), context);

    char const* version = client.Version();
    ASSERT_TRUE(version);
    ASSERT_STREQ(version, "3.0.10");  // {x-release-please-version}
}

TEST(ClientTest, AllFlagsIsEmpty) {
    Client client(ConfigBuilder("sdk-123").Build().value(),
                  ContextBuilder().Kind("cat", "shadow").Build());

    ASSERT_TRUE(client.AllFlags().empty());
}

TEST(ClientTest, BoolVariationDefaultPassesThrough) {
    Client client(ConfigBuilder("sdk-123").Build().value(),
                  ContextBuilder().Kind("cat", "shadow").Build());

    const std::string flag = "extra-cat-food";
    std::vector<bool> values = {true, false};
    for (auto const& v : values) {
        ASSERT_EQ(client.BoolVariation(flag, v), v);
        ASSERT_EQ(*client.BoolVariationDetail(flag, v), v);
    }
}

TEST(ClientTest, StringVariationDefaultPassesThrough) {
    Client client(ConfigBuilder("sdk-123").Build().value(),
                  ContextBuilder().Kind("cat", "shadow").Build());
    const std::string flag = "treat";
    std::vector<std::string> values = {"chicken", "fish", "cat-grass"};
    for (auto const& v : values) {
        ASSERT_EQ(client.StringVariation(flag, v), v);
        ASSERT_EQ(*client.StringVariationDetail(flag, v), v);
    }
}

TEST(ClientTest, IntVariationDefaultPassesThrough) {
    Client client(ConfigBuilder("sdk-123").Build().value(),
                  ContextBuilder().Kind("cat", "shadow").Build());
    const std::string flag = "weight";
    std::vector<int> values = {0, 12, 13, 24, 1000};
    for (auto const& v : values) {
        ASSERT_EQ(client.IntVariation("weight", v), v);
        ASSERT_EQ(*client.IntVariationDetail("weight", v), v);
    }
}

TEST(ClientTest, DoubleVariationDefaultPassesThrough) {
    Client client(ConfigBuilder("sdk-123").Build().value(),
                  ContextBuilder().Kind("cat", "shadow").Build());
    const std::string flag = "weight";
    std::vector<double> values = {0.0, 12.0, 13.0, 24.0, 1000.0};
    for (auto const& v : values) {
        ASSERT_EQ(client.DoubleVariation(flag, v), v);
        ASSERT_EQ(*client.DoubleVariationDetail(flag, v), v);
    }
}

TEST(ClientTest, JsonVariationDefaultPassesThrough) {
    Client client(ConfigBuilder("sdk-123").Build().value(),
                  ContextBuilder().Kind("cat", "shadow").Build());

    const std::string flag = "assorted-values";
    std::vector<Value> values = {
        Value({"running", "jumping"}), Value(3), Value(1.0), Value(true),
        Value(std::map<std::string, Value>{{"weight", 20}})};
    for (auto const& v : values) {
        ASSERT_EQ(client.JsonVariation(flag, v), v);
        ASSERT_EQ(*client.JsonVariationDetail(flag, v), v);
    }
}
