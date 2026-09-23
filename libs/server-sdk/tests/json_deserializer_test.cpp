#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "data_components/serialization_adapters/json_deserializer.hpp"
#include "spy_logger.hpp"

using namespace launchdarkly;
using namespace launchdarkly::server_side;

namespace {

using ItemMap = std::unordered_map<std::string, std::string>;

/* The three tombstone shapes our SDKs write. A store addresses a record by key
 * on its own, so the key in the body is redundant and not every SDK writes
 * it. */
std::string const kKeylessTombstone = R"({"version":100,"deleted":true})";
std::string const kKeyedTombstone =
    R"({"key":"item-key","version":100,"deleted":true})";
std::string const kPlaceholderFlagTombstone =
    R"({"key":"$deleted","version":100,"deleted":true,"on":true,)"
    R"("variations":["a","b"],"fallthrough":{"variation":0}})";
std::string const kPlaceholderSegmentTombstone =
    R"({"key":"$deleted","version":100,"deleted":true,)"
    R"("included":["user-key"]})";

std::string const kLiveFlag =
    R"({"key":"item-key","version":100,"on":true,"variations":["a","b"],)"
    R"("fallthrough":{"variation":0}})";
std::string const kLiveSegment =
    R"({"key":"item-key","version":100,"included":["user-key"]})";

/* Serves preloaded JSON bodies the way a persistent store does: addressed by
 * the key the record is stored under, and with no out-of-band deletion state,
 * so deletion can only be discovered by reading the body. */
class FakeDataReader final : public integrations::ISerializedDataReader {
   public:
    FakeDataReader(ItemMap flags, ItemMap segments)
        : flags_(std::move(flags)), segments_(std::move(segments)) {}

    [[nodiscard]] GetResult Get(integrations::ISerializedItemKind const& kind,
                                std::string const& itemKey) const override {
        auto const& items = ItemsFor(kind);
        auto const it = items.find(itemKey);
        if (it == items.end()) {
            return std::nullopt;
        }
        return integrations::SerializedItemDescriptor::Present(0, it->second);
    }

    [[nodiscard]] AllResult All(
        integrations::ISerializedItemKind const& kind) const override {
        AllResult::value_type out;
        for (auto const& [key, body] : ItemsFor(kind)) {
            out.emplace(
                key, integrations::SerializedItemDescriptor::Present(0, body));
        }
        return out;
    }

    [[nodiscard]] std::string const& Identity() const override {
        return identity_;
    }

    [[nodiscard]] bool Initialized() const override { return true; }

   private:
    [[nodiscard]] ItemMap const& ItemsFor(
        integrations::ISerializedItemKind const& kind) const {
        return kind.Namespace() == "features" ? flags_ : segments_;
    }

    ItemMap const flags_;
    ItemMap const segments_;
    std::string const identity_ = "fake reader";
};

class JsonDeserializerTest : public ::testing::Test {
   public:
    JsonDeserializerTest()
        : spy_logger_backend(std::make_shared<logging::SpyLoggerBackend>()),
          logger(spy_logger_backend) {}

    data_components::JsonDeserializer WithFlags(ItemMap flags) {
        return data_components::JsonDeserializer{
            logger,
            std::make_shared<FakeDataReader>(std::move(flags),
                                             /* segments= */ ItemMap{})};
    }

    data_components::JsonDeserializer WithSegments(ItemMap segments) {
        return data_components::JsonDeserializer{
            logger, std::make_shared<FakeDataReader>(/* flags= */ ItemMap{},
                                                     std::move(segments))};
    }

    std::shared_ptr<logging::SpyLoggerBackend> const spy_logger_backend;
    Logger const logger;
};

}  // namespace

TEST_F(JsonDeserializerTest, ReadsFlagTombstoneWithNoKey) {
    auto const reader = WithFlags({{"item-key", kKeylessTombstone}});

    auto const flag = reader.GetFlag("item-key");
    ASSERT_TRUE(flag);
    ASSERT_TRUE(*flag);
    EXPECT_FALSE((*flag)->item.has_value());
    EXPECT_EQ((*flag)->version, 100);
}

TEST_F(JsonDeserializerTest, ReadsFlagTombstoneRepeatingTheItemKey) {
    auto const reader = WithFlags({{"item-key", kKeyedTombstone}});

    auto const flag = reader.GetFlag("item-key");
    ASSERT_TRUE(flag);
    ASSERT_TRUE(*flag);
    EXPECT_FALSE((*flag)->item.has_value());
    EXPECT_EQ((*flag)->version, 100);
}

TEST_F(JsonDeserializerTest, ReadsFlagTombstoneWithPlaceholderKey) {
    auto const reader = WithFlags({{"item-key", kPlaceholderFlagTombstone}});

    auto const flag = reader.GetFlag("item-key");
    ASSERT_TRUE(flag);
    ASSERT_TRUE(*flag);
    EXPECT_FALSE((*flag)->item.has_value());
    EXPECT_EQ((*flag)->version, 100);
}

TEST_F(JsonDeserializerTest, ReadsLiveFlag) {
    auto const reader = WithFlags({{"item-key", kLiveFlag}});

    auto const flag = reader.GetFlag("item-key");
    ASSERT_TRUE(flag);
    ASSERT_TRUE(*flag);
    ASSERT_TRUE((*flag)->item.has_value());
    EXPECT_EQ((*flag)->item->key, "item-key");
    EXPECT_EQ((*flag)->version, 100);
}

TEST_F(JsonDeserializerTest, ReadsFlagMarkedNotDeletedAsLive) {
    auto const reader = WithFlags(
        {{"item-key",
          R"({"key":"item-key","version":100,"deleted":false,"on":true})"}});

    auto const flag = reader.GetFlag("item-key");
    ASSERT_TRUE(flag);
    ASSERT_TRUE(*flag);
    ASSERT_TRUE((*flag)->item.has_value());
    EXPECT_EQ((*flag)->item->key, "item-key");
}

TEST_F(JsonDeserializerTest, AllFlagsKeepsLiveFlagAlongsideTombstones) {
    auto const reader =
        WithFlags({{"live-key", kLiveFlag},
                   {"keyless-key", kKeylessTombstone},
                   {"keyed-key", kKeyedTombstone},
                   {"placeholder-key", kPlaceholderFlagTombstone}});

    auto const flags = reader.AllFlags();
    ASSERT_TRUE(flags);
    ASSERT_EQ(flags->size(), 4);
    EXPECT_TRUE(flags->at("live-key").item.has_value());
    EXPECT_FALSE(flags->at("keyless-key").item.has_value());
    EXPECT_FALSE(flags->at("keyed-key").item.has_value());
    EXPECT_FALSE(flags->at("placeholder-key").item.has_value());
}

TEST_F(JsonDeserializerTest, ReadsSegmentTombstoneWithNoKey) {
    auto const reader = WithSegments({{"item-key", kKeylessTombstone}});

    auto const segment = reader.GetSegment("item-key");
    ASSERT_TRUE(segment);
    ASSERT_TRUE(*segment);
    EXPECT_FALSE((*segment)->item.has_value());
    EXPECT_EQ((*segment)->version, 100);
}

TEST_F(JsonDeserializerTest, ReadsSegmentTombstoneRepeatingTheItemKey) {
    auto const reader = WithSegments({{"item-key", kKeyedTombstone}});

    auto const segment = reader.GetSegment("item-key");
    ASSERT_TRUE(segment);
    ASSERT_TRUE(*segment);
    EXPECT_FALSE((*segment)->item.has_value());
    EXPECT_EQ((*segment)->version, 100);
}

TEST_F(JsonDeserializerTest, ReadsSegmentTombstoneWithPlaceholderKey) {
    auto const reader =
        WithSegments({{"item-key", kPlaceholderSegmentTombstone}});

    auto const segment = reader.GetSegment("item-key");
    ASSERT_TRUE(segment);
    ASSERT_TRUE(*segment);
    EXPECT_FALSE((*segment)->item.has_value());
    EXPECT_EQ((*segment)->version, 100);
}

TEST_F(JsonDeserializerTest, ReadsLiveSegment) {
    auto const reader = WithSegments({{"item-key", kLiveSegment}});

    auto const segment = reader.GetSegment("item-key");
    ASSERT_TRUE(segment);
    ASSERT_TRUE(*segment);
    ASSERT_TRUE((*segment)->item.has_value());
    EXPECT_EQ((*segment)->item->key, "item-key");
    EXPECT_EQ((*segment)->version, 100);
}

TEST_F(JsonDeserializerTest, AllSegmentsKeepsLiveSegmentAlongsideTombstones) {
    auto const reader =
        WithSegments({{"live-key", kLiveSegment},
                      {"keyless-key", kKeylessTombstone},
                      {"keyed-key", kKeyedTombstone},
                      {"placeholder-key", kPlaceholderSegmentTombstone}});

    auto const segments = reader.AllSegments();
    ASSERT_TRUE(segments);
    ASSERT_EQ(segments->size(), 4);
    EXPECT_TRUE(segments->at("live-key").item.has_value());
    EXPECT_FALSE(segments->at("keyless-key").item.has_value());
    EXPECT_FALSE(segments->at("keyed-key").item.has_value());
    EXPECT_FALSE(segments->at("placeholder-key").item.has_value());
}
