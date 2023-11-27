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
};

TEST(JsonDestination, WrapsUnderlyingDestinationIdentity) {
    auto logger = logging::NullLogger();
    NiceMock<MockSerializedDestination> mock_dest;
    JsonDestination const destination{logger, mock_dest};

    EXPECT_CALL(mock_dest, Identity()).WillOnce([]() {
        return "FooCorp Database";
    });

    ASSERT_EQ(destination.Identity(), "FooCorp Database (JSON)");
}

TEST(JsonDestination, InitErrorGeneratesLogMessage) {
    auto spy_logger = std::make_shared<logging::SpyLoggerBackend>();
    Logger logger(spy_logger);

    NiceMock<MockSerializedDestination> mock_dest;
    JsonDestination destination{logger, mock_dest};

    EXPECT_CALL(mock_dest, Identity()).WillOnce([] {
        return "FooCorp Database";
    });

    EXPECT_CALL(mock_dest, Init)
        .WillOnce(Return(ISerializedDestination::InitResult::kError));

    destination.Init(data_model::SDKDataSet{});

    ASSERT_TRUE(spy_logger->Contains(0, LogLevel::kError, "failed"));
}

// The SerializedDestination need only be concerned with inserting items
// exactly as specified in its Init argument. This test verifies that the
// transformation from SDKDataSet into that argument is done correctly. The
// transformation consists of two parts; first, serializing items to JSON, and
// second, ordering those items by comparing keys with '<'.
TEST(JsonDestination, InitProperlyTransformsSDKDataSet) {
    auto logger = logging::NullLogger();
    MockSerializedDestination mock_dest;
    JsonDestination destination{logger, mock_dest};

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

    // Note: flag/segments are out of alphabetical order here to help verify
    // they are sorted.
    destination.Init(data_model::SDKDataSet{
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

TEST(JsonDestination, UpsertFlagErrorGeneratesErrorMessage) {
    auto spy_logger = std::make_shared<logging::SpyLoggerBackend>();
    Logger logger(spy_logger);

    NiceMock<MockSerializedDestination> mock_dest;
    JsonDestination destination{logger, mock_dest};

    EXPECT_CALL(mock_dest, Identity()).WillOnce([] {
        return "FooCorp Database";
    });

    EXPECT_CALL(mock_dest, Upsert)
        .WillOnce(Return(ISerializedDestination::UpsertResult::kError));

    destination.Upsert("foo", data_model::FlagDescriptor(1));

    ASSERT_TRUE(
        spy_logger->Contains(0, LogLevel::kError, "failed to update flag foo"));
}

TEST(JsonDestination, UpsertSegmentErrorGeneratesErrorMessage) {
    auto spy_logger = std::make_shared<logging::SpyLoggerBackend>();
    Logger logger(spy_logger);

    NiceMock<MockSerializedDestination> mock_dest;
    JsonDestination destination{logger, mock_dest};

    EXPECT_CALL(mock_dest, Identity()).WillOnce([] {
        return "FooCorp Database";
    });

    EXPECT_CALL(mock_dest, Upsert)
        .WillOnce(Return(ISerializedDestination::UpsertResult::kError));

    destination.Upsert("foo", data_model::SegmentDescriptor(1));

    ASSERT_TRUE(spy_logger->Contains(0, LogLevel::kError,
                                     "failed to update segment foo"));
}

TEST(JsonDestination, UpsertStaleFlagGeneratesDebugMessage) {
    auto spy_logger = std::make_shared<logging::SpyLoggerBackend>();
    Logger logger(spy_logger);

    NiceMock<MockSerializedDestination> mock_dest;
    JsonDestination destination{logger, mock_dest};

    EXPECT_CALL(mock_dest, Identity()).WillOnce([] {
        return "FooCorp Database";
    });

    EXPECT_CALL(mock_dest, Upsert)
        .WillOnce(Return(ISerializedDestination::UpsertResult::kNotUpdated));

    destination.Upsert("foo", data_model::FlagDescriptor(1));

    ASSERT_TRUE(
        spy_logger->Contains(0, LogLevel::kDebug, "flag foo not updated"));
}

TEST(JsonDestination, UpsertStaleSegmentGeneratesDebugMessage) {
    auto spy_logger = std::make_shared<logging::SpyLoggerBackend>();
    Logger logger(spy_logger);

    NiceMock<MockSerializedDestination> mock_dest;
    JsonDestination destination{logger, mock_dest};

    EXPECT_CALL(mock_dest, Identity()).WillOnce([] {
        return "FooCorp Database";
    });

    EXPECT_CALL(mock_dest, Upsert)
        .WillOnce(Return(ISerializedDestination::UpsertResult::kNotUpdated));

    destination.Upsert("foo", data_model::SegmentDescriptor(1));

    ASSERT_TRUE(
        spy_logger->Contains(0, LogLevel::kDebug, "segment foo not updated"));
}
