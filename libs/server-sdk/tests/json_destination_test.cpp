#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "data_components/serialization_adapters/json_destination.hpp"

#include <launchdarkly/logging/null_logger.hpp>

#include "spy_logger.hpp"

using namespace launchdarkly;
using namespace launchdarkly::server_side::data_components;
using namespace launchdarkly::server_side::data_interfaces;
using namespace launchdarkly::server_side::integrations;

using ::testing::Eq;
using ::testing::NiceMock;
using ::testing::Pair;
using ::testing::Ref;
using ::testing::Return;

class MockSerializedDestination : public ISerializedDestination {
   public:
    MOCK_METHOD(InitResult, Init, (std::vector<ItemCollection>), (override));
    MOCK_METHOD(UpsertResult,
                Upsert,
                (ISerializedItemKind const&,
                 std::string const&,
                 SerializedItemDescriptor),
                (override));
    MOCK_METHOD(std::string const&, Identity, (), (const, override));
    MockSerializedDestination() {
        ON_CALL(*this, Identity).WillByDefault(testing::ReturnRef(name));
    }

   private:
    std::string const name = "FooCorp Database";
};

class JsonDestinationTest : public ::testing::Test {
   public:
    std::shared_ptr<logging::SpyLoggerBackend> spy_logger;
    Logger const logger;
    NiceMock<MockSerializedDestination> mock_dest;
    JsonDestination dest;
    JsonDestinationTest()
        : spy_logger(std::make_shared<logging::SpyLoggerBackend>()),
          logger(spy_logger),
          dest(logger, mock_dest) {}
};

TEST_F(JsonDestinationTest, WrapsUnderlyingDestinationIdentity) {
    ASSERT_EQ(dest.Identity(), "FooCorp Database (JSON)");
}

TEST_F(JsonDestinationTest, InitErrorGeneratesLogMessage) {
    EXPECT_CALL(mock_dest, Init)
        .WillOnce(Return(ISerializedDestination::InitResult::kError));

    dest.Init(data_model::SDKDataSet{});

    ASSERT_TRUE(spy_logger->Contains(0, LogLevel::kError, "failed"));
}

// The SerializedDestination need only be concerned with inserting items
// exactly as specified in its Init argument. This test verifies that the
// transformation from SDKDataSet into that argument is done correctly. The
// transformation consists of two parts; first, serializing items to JSON, and
// second, ordering those items by comparing keys with '<'.
TEST_F(JsonDestinationTest, InitProperlyTransformsSDKDataSet) {
    EXPECT_CALL(
        mock_dest,
        Init(Eq(std::vector<ISerializedDestination::ItemCollection>{
            {{JsonDestination::Kinds::Flag,
              std::vector<
                  ISerializedDestination::Keyed<SerializedItemDescriptor>>{
                  {"flag_alpha",
                   SerializedItemDescriptor::Present(
                       1, "{\"key\":\"flag_alpha\",\"version\":1}")},
                  {"flag_beta",
                   SerializedItemDescriptor::Present(
                       2, "{\"key\":\"flag_beta\",\"version\":2}")}}},
             {JsonDestination::Kinds::Segment,
              std::vector<
                  ISerializedDestination::Keyed<SerializedItemDescriptor>>{
                  {"segment_alpha",
                   SerializedItemDescriptor::Present(
                       1, "{\"key\":\"segment_alpha\",\"version\":1}")},
                  {"segment_beta",
                   SerializedItemDescriptor::Present(
                       2, "{\"key\":\"segment_beta\",\"version\":2}")}}}}})))
        .WillOnce(Return(ISerializedDestination::InitResult::kSuccess));

    // Note: flag/segments are deliberately not in alphabetical order here,
    // so that the implementation must sort them to pass the test.
    dest.Init(data_model::SDKDataSet{
        std::unordered_map<std::string, data_model::FlagDescriptor>{
            {"flag_beta",
             data_model::FlagDescriptor(data_model::Flag{"flag_beta", 2})},
            {"flag_alpha",
             data_model::FlagDescriptor(data_model::Flag{"flag_alpha", 1})}},
        std::unordered_map<std::string, data_model::SegmentDescriptor>{
            {"segment_beta", data_model::SegmentDescriptor(
                                 data_model::Segment{"segment_beta", 2})},
            {"segment_alpha", data_model::SegmentDescriptor(
                                  data_model::Segment{"segment_alpha", 1})}}});
}

TEST_F(JsonDestinationTest, UpsertFlagErrorGeneratesErrorMessage) {
    EXPECT_CALL(mock_dest, Upsert)
        .WillOnce(Return(ISerializedDestination::UpsertResult::kError));

    dest.Upsert("foo", data_model::FlagDescriptor(data_model::Tombstone(1)));

    ASSERT_TRUE(
        spy_logger->Contains(0, LogLevel::kError, "failed to update flag foo"));
}

TEST_F(JsonDestinationTest, UpsertSegmentErrorGeneratesErrorMessage) {
    EXPECT_CALL(mock_dest, Upsert)
        .WillOnce(Return(ISerializedDestination::UpsertResult::kError));

    dest.Upsert("foo", data_model::SegmentDescriptor(data_model::Tombstone(1)));

    ASSERT_TRUE(spy_logger->Contains(0, LogLevel::kError,
                                     "failed to update segment foo"));
}

TEST_F(JsonDestinationTest, UpsertStaleFlagGeneratesDebugMessage) {
    EXPECT_CALL(mock_dest, Upsert)
        .WillOnce(Return(ISerializedDestination::UpsertResult::kNotUpdated));

    dest.Upsert("foo", data_model::FlagDescriptor(data_model::Tombstone(1)));

    ASSERT_TRUE(
        spy_logger->Contains(0, LogLevel::kDebug, "flag foo not updated"));
}

TEST_F(JsonDestinationTest, UpsertStaleSegmentGeneratesDebugMessage) {
    EXPECT_CALL(mock_dest, Upsert)
        .WillOnce(Return(ISerializedDestination::UpsertResult::kNotUpdated));

    dest.Upsert("foo", data_model::SegmentDescriptor(data_model::Tombstone(1)));

    ASSERT_TRUE(
        spy_logger->Contains(0, LogLevel::kDebug, "segment foo not updated"));
}

TEST_F(JsonDestinationTest, UpsertDeletedFlagCreatesTombstone) {
    EXPECT_CALL(
        mock_dest,
        Upsert(Ref(JsonDestination::Kinds::Flag), "flag",
               SerializedItemDescriptor::Tombstone(
                   2, "{\"key\":\"flag\",\"version\":2,\"deleted\":true}")))
        .WillOnce(Return(ISerializedDestination::UpsertResult::kSuccess));

    dest.Upsert("flag", data_model::FlagDescriptor(data_model::Tombstone(2)));
}

TEST_F(JsonDestinationTest, UpsertDeletedSegmentCreatesTombstone) {
    EXPECT_CALL(
        mock_dest,
        Upsert(Ref(JsonDestination::Kinds::Segment), "segment",
               SerializedItemDescriptor::Tombstone(
                   2, "{\"key\":\"segment\",\"version\":2,\"deleted\":true}")))
        .WillOnce(Return(ISerializedDestination::UpsertResult::kSuccess));

    dest.Upsert("segment",
                data_model::SegmentDescriptor(data_model::Tombstone(2)));
}

TEST_F(JsonDestinationTest, UpsertFlagCreatesSerializedItem) {
    EXPECT_CALL(mock_dest,
                Upsert(Ref(JsonDestination::Kinds::Flag), "flag",
                       SerializedItemDescriptor::Present(
                           2, "{\"on\":true,\"key\":\"flag\",\"version\":2}")))
        .WillOnce(Return(ISerializedDestination::UpsertResult::kSuccess));

    dest.Upsert("flag",
                data_model::FlagDescriptor(data_model::Flag{"flag", 2, true}));
}

TEST_F(JsonDestinationTest, UpsertSegmentCreatesSerializedItem) {
    EXPECT_CALL(mock_dest,
                Upsert(Ref(JsonDestination::Kinds::Segment), "segment",
                       SerializedItemDescriptor::Present(
                           2,
                           "{\"key\":\"segment\",\"version\":2,\"excluded\":"
                           "[\"bar\"],\"included\":[\"foo\"]}")))
        .WillOnce(Return(ISerializedDestination::UpsertResult::kSuccess));

    dest.Upsert("segment", data_model::SegmentDescriptor(data_model::Segment{
                               "segment", 2, {"foo"}, {"bar"}}));
}
