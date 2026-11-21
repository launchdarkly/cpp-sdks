#include <gtest/gtest.h>
#include <launchdarkly/client_side/client.hpp>
#include <launchdarkly/context_builder.hpp>
#include <map>
#include <memory>
#include <optional>
#include <string>

using namespace launchdarkly;
using namespace launchdarkly::client_side;

namespace {

/* Returns the same flag data for every context. */
class FlagDataPersistence : public IPersistence {
   public:
    explicit FlagDataPersistence(std::string flag_data)
        : flag_data_(std::move(flag_data)) {}

    void Set(std::string, std::string, std::string) noexcept override {}

    void Remove(std::string, std::string) noexcept override {}

    std::optional<std::string> Read(std::string,
                                    std::string key) noexcept override {
        if (key == "ContextIndex") {
            return std::nullopt;
        }
        return flag_data_;
    }

   private:
    std::string flag_data_;
};

}  // namespace

TEST(ClientTest, ClientConstructedWithMinimalConfigAndContext) {
    tl::expected<Config, Error> config = ConfigBuilder("sdk-123").Build();
    ASSERT_TRUE(config);

    Context context = ContextBuilder().Kind("cat", "shadow").Build();

    Client client(std::move(*config), context);

    char const* version = client.Version();
    ASSERT_TRUE(version);
    ASSERT_STREQ(version, "3.13.3");  // {x-release-please-version}
}

TEST(ClientTest, AllFlagsIsEmpty) {
    Client client(ConfigBuilder("sdk-123").Build().value(),
                  ContextBuilder().Kind("cat", "shadow").Build());

    ASSERT_TRUE(client.AllFlags().empty());
}

TEST(ClientTest, BoolVariationDefaultPassesThrough) {
    Client client(ConfigBuilder("sdk-123").Build().value(),
                  ContextBuilder().Kind("cat", "shadow").Build());

    std::string const flag = "extra-cat-food";
    std::vector<bool> values = {true, false};
    for (auto const& v : values) {
        ASSERT_EQ(client.BoolVariation(flag, v), v);
        ASSERT_EQ(*client.BoolVariationDetail(flag, v), v);
    }
}

TEST(ClientTest, StringVariationDefaultPassesThrough) {
    Client client(ConfigBuilder("sdk-123").Build().value(),
                  ContextBuilder().Kind("cat", "shadow").Build());
    std::string const flag = "treat";
    std::vector<std::string> values = {"chicken", "fish", "cat-grass"};
    for (auto const& v : values) {
        ASSERT_EQ(client.StringVariation(flag, v), v);
        ASSERT_EQ(*client.StringVariationDetail(flag, v), v);
    }
}

TEST(ClientTest, IntVariationDefaultPassesThrough) {
    Client client(ConfigBuilder("sdk-123").Build().value(),
                  ContextBuilder().Kind("cat", "shadow").Build());
    std::string const flag = "weight";
    std::vector<int> values = {0, 12, 13, 24, 1000};
    for (auto const& v : values) {
        ASSERT_EQ(client.IntVariation("weight", v), v);
        ASSERT_EQ(*client.IntVariationDetail("weight", v), v);
    }
}

TEST(ClientTest, DoubleVariationDefaultPassesThrough) {
    Client client(ConfigBuilder("sdk-123").Build().value(),
                  ContextBuilder().Kind("cat", "shadow").Build());
    std::string const flag = "weight";
    std::vector<double> values = {0.0, 12.0, 13.0, 24.0, 1000.0};
    for (auto const& v : values) {
        ASSERT_EQ(client.DoubleVariation(flag, v), v);
        ASSERT_EQ(*client.DoubleVariationDetail(flag, v), v);
    }
}

TEST(ClientTest, JsonVariationDefaultPassesThrough) {
    Client client(ConfigBuilder("sdk-123").Build().value(),
                  ContextBuilder().Kind("cat", "shadow").Build());

    std::string const flag = "assorted-values";
    std::vector<Value> values = {
        Value({"running", "jumping"}), Value(3), Value(1.0), Value(true),
        Value(std::map<std::string, Value>{{"weight", 20}})};
    for (auto const& v : values) {
        ASSERT_EQ(client.JsonVariation(flag, v), v);
        ASSERT_EQ(*client.JsonVariationDetail(flag, v), v);
    }
}

TEST(ClientTest, JsonVariationReturnsDefaultWhenFlagHasNoValue) {
    // The cached data has a flag with a null value and a flag without a
    // value. Each flag has no variation.
    auto builder = ConfigBuilder("sdk-123");
    builder.Offline(true);
    builder.Persistence().Custom(std::make_shared<FlagDataPersistence>(
        R"({"null-value":{"version":1,"value":null,"variation":null,)"
        R"("reason":{"kind":"OFF"}},)"
        R"("omitted-value":{"version":1,"reason":{"kind":"OFF"}}})"));
    Client client(builder.Build().value(),
                  ContextBuilder().Kind("cat", "shadow").Build());

    // The JSON variation calls return the default value for each flag.
    for (auto const& flag : {"null-value", "omitted-value"}) {
        EXPECT_EQ(client.JsonVariation(flag, "default"), Value("default"))
            << flag;
        auto const detail = client.JsonVariationDetail(flag, "default");
        EXPECT_EQ(detail.Value(), Value("default")) << flag;
        EXPECT_EQ(detail.Reason(), EvaluationReason::Off()) << flag;
    }
}

// This test mainly serves to catch any changes made to the types in the Data
// Source Status API that are not backwards-compatible.
TEST(ClientTest, DataSourceStatus) {
    Client client(ConfigBuilder("sdk-123").Build().value(),
                  ContextBuilder().Kind("cat", "shadow").Build());

    client_side::data_sources::DataSourceStatus ds_status =
        client.DataSourceStatus().Status();

    std::optional<client_side::data_sources::DataSourceStatus::ErrorInfo>
        last_err = ds_status.LastError();

    ASSERT_FALSE(last_err);

    client_side::data_sources::DataSourceStatus::DataSourceState state =
        ds_status.State();

    ASSERT_EQ(state, client_side::data_sources::DataSourceStatus::
                         DataSourceState::kInitializing);

    client_side::data_sources::DataSourceStatus::DateTime date =
        ds_status.StateSince();

    ASSERT_NE(date, client_side::data_sources::DataSourceStatus::DateTime{});
}
